from __future__ import annotations

import numpy as np
from scipy.signal import butter, filtfilt

from proto.chaos.aizawa import Aizawa
from proto.chaos.chua import Chua
from proto.chaos.duffing import Duffing
from proto.chaos.logistic import Logistic
from proto.chaos.lorenz import Lorenz
from proto.chaos.rossler import Rossler
from proto.chaos.thomas import Thomas
from proto.effects.granular import granular
from proto.effects.svf import svf, svf_morph
from proto.effects.wavefolder import wavefold
from proto.macros import resolve_macros
from proto.preset import Preset


# Axis names each core exposes as modulation sources.
CORE_AXES = {
    "lorenz":   ("x", "y", "z"),
    "rossler":  ("x", "y", "z"),
    "thomas":   ("x", "y", "z"),
    "chua":     ("x", "y", "z"),
    "aizawa":   ("x", "y", "z"),
    "duffing":  ("x", "v"),
    "logistic": ("x",),
}


def _make_core(spec, audio, sample_rate):
    t = spec.type
    # Macros resolved first; raw params override (power-user escape hatch).
    p = {**resolve_macros(t, spec.macros), **spec.params}
    smoothing_hz = float(p.pop("smoothing_hz", 0.0))

    if t == "lorenz":
        core = Lorenz(**{k: p[k] for k in ("sigma", "rho", "beta", "speed", "init") if k in p})
        arr = Lorenz.normalize(core.generate(len(audio), sample_rate))
    elif t == "rossler":
        core = Rossler(**{k: p[k] for k in ("a", "b", "c", "speed", "init") if k in p})
        arr = Rossler.normalize(core.generate(len(audio), sample_rate))
    elif t == "thomas":
        core = Thomas(**{k: p[k] for k in ("b", "speed", "init") if k in p})
        arr = Thomas.normalize(core.generate(len(audio), sample_rate))
    elif t == "chua":
        core = Chua(**{k: p[k] for k in ("alpha", "beta", "m0", "m1", "speed", "init") if k in p})
        arr = Chua.normalize(core.generate(len(audio), sample_rate))
    elif t == "aizawa":
        core = Aizawa(**{k: p[k] for k in ("a", "b", "c", "d", "e", "f", "speed", "init") if k in p})
        arr = Aizawa.normalize(core.generate(len(audio), sample_rate))
    elif t == "duffing":
        input_drive = p.pop("input_drive", 0.0)
        core = Duffing(**{k: p[k] for k in ("delta", "alpha", "beta", "gamma", "omega", "speed", "init") if k in p})
        raw = core.generate(len(audio), sample_rate,
                            input_signal=audio if input_drive else None,
                            input_drive=input_drive)
        arr = Duffing.normalize(raw)
    elif t == "logistic":
        core = Logistic(**{k: p[k] for k in ("r", "step_rate", "x0") if k in p})
        arr = Logistic.normalize(core.generate(len(audio), sample_rate))[:, None]
    else:
        raise ValueError(f"unknown core type: {t}")

    if smoothing_hz > 0.0:
        b, a_lp = butter(2, smoothing_hz / (sample_rate / 2.0))
        for col in range(arr.shape[1]):
            arr[:, col] = filtfilt(b, a_lp, arr[:, col])
    return arr


def _build_mod_streams(preset, audio, sample_rate):
    streams = {}
    for spec in preset.cores:
        arr = _make_core(spec, audio, sample_rate)
        axes = CORE_AXES[spec.type]
        for i, name in enumerate(axes):
            streams[f"{spec.id}.{name}"] = arr[:, i] if arr.ndim > 1 else arr
    return streams


def _apply_mod(base_array, mod_signal, mod_spec):
    m = mod_signal
    if mod_spec.range == "positive":
        m = (m + 1.0) * 0.5            # [-1,1] -> [0,1]
    if mod_spec.scale == "exp2":
        return base_array * np.power(2.0, m * mod_spec.depth)
    return base_array + m * mod_spec.depth


def _run_effect(spec, signal, params_arr, sample_rate):
    # params_arr: dict of per-sample np arrays or scalars for this effect's modulated params.
    t = spec.type

    def _param(name, default=None):
        if name in params_arr:
            return params_arr[name]
        if name in spec.params:
            return spec.params[name]
        return default

    if t == "wavefolder":
        return wavefold(signal, _param("drive", 1.0))

    if t == "svf":
        mode = spec.params.get("mode", "lp")
        lp, bp, hp = svf(signal, _param("cutoff", 1000.0), _param("resonance", 0.3), sample_rate)
        return {"lp": lp, "bp": bp, "hp": hp}[mode]

    if t == "svf_morph":
        return svf_morph(signal, _param("cutoff", 1000.0), _param("resonance", 0.3),
                         _param("morph", 0.5), sample_rate)

    if t == "granular":
        # Granular sees arrays where present, scalars otherwise.
        pos = _param("position", None)
        pitch = _param("pitch", None)
        # Scalar-only params; take mean if somehow modulated.
        def _scalar(v, fallback):
            if v is None:
                return fallback
            if np.ndim(v) == 0:
                return float(v)
            return float(np.mean(v))
        grain_size_ms = _scalar(_param("grain_size_ms"), 100.0)
        overlap = int(spec.params.get("overlap", 4))
        jitter = _scalar(_param("jitter"), 0.0)
        pos_arr = pos if (pos is not None and np.ndim(pos) > 0) else None
        pitch_arr = pitch if (pitch is not None and np.ndim(pitch) > 0) else None
        return granular(signal, sample_rate,
                        grain_size_ms=grain_size_ms, overlap=overlap,
                        position_mod=pos_arr, pitch_mod=pitch_arr, jitter=jitter)

    raise ValueError(f"unknown effect type: {t}")


def run_preset(preset: Preset, audio: np.ndarray, sample_rate: int) -> np.ndarray:
    n = len(audio)
    a = audio.astype(np.float64)

    mod_streams = _build_mod_streams(preset, a, sample_rate)

    # Initialize per-effect param buffers with base values.
    effect_params = {}
    for eff in preset.chain:
        effect_params[eff.id] = {k: np.full(n, float(v), dtype=np.float64)
                                 for k, v in eff.params.items()
                                 if isinstance(v, (int, float))}

    # Apply modulation.
    for mod in preset.modulation:
        eff_id, param_name = mod.dst.split(".")
        if eff_id not in effect_params:
            raise KeyError(f"mod dst references unknown effect id: {eff_id!r}")
        if mod.src not in mod_streams:
            raise KeyError(f"mod src {mod.src!r} not in streams {list(mod_streams)}")
        base = effect_params[eff_id].get(param_name)
        if base is None:
            base = np.full(n, 0.0)
        effect_params[eff_id][param_name] = _apply_mod(base, mod_streams[mod.src], mod)

    # Run chain.
    signal = a
    for eff in preset.chain:
        signal = _run_effect(eff, signal, effect_params[eff.id], sample_rate)

    gain = float(np.power(10.0, preset.output_gain_db / 20.0))
    signal = signal * gain

    peak = float(np.max(np.abs(signal)))
    if peak > 1e-9:
        signal = signal * (0.9 / peak)
    return signal.astype(np.float32)
