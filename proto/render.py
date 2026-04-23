import argparse
import sys
from pathlib import Path

import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

from proto.engine import run_preset
from proto.preset import Preset, apply_overrides, load_presets
from proto.sources import synth_bass_fm, synth_bass_saw, synth_bass_sub

SAMPLE_RATE = 48000
DURATION = 4.0


def load_sources(input_dir: Path, sample_rate: int, include_synth: bool = True) -> dict:
    sources = {}
    if include_synth:
        sources["saw_bass"] = synth_bass_saw(41.2, DURATION, sample_rate)  # E1
        sources["fm_bass"] = synth_bass_fm(55.0, DURATION, sample_rate)    # A1
        sources["sub_bass"] = synth_bass_sub(32.7, DURATION, sample_rate)  # C1

    if input_dir.exists():
        for wav in sorted(input_dir.glob("*.wav")):
            audio, sr = sf.read(str(wav))
            if audio.ndim > 1:
                audio = audio.mean(axis=1)
            if sr != sample_rate:
                audio = resample_poly(audio, sample_rate, sr)
            sources[f"user_{wav.stem}"] = audio.astype(np.float32)

    return sources


def parse_args(argv=None):
    p = argparse.ArgumentParser(
        description="Render sources through chaos-driven FX presets.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    p.add_argument("--preset", action="append", default=None,
                   help="Preset name (repeatable). Default: all presets.")
    p.add_argument("--source", action="append", default=None,
                   help="Source name (stem) (repeatable). Default: all sources.")
    p.add_argument("--override", action="append", default=[],
                   help="Per-render param override, e.g. --override 'cores.lor1.speed=2.5'. "
                        "Repeatable. Applied to every rendered preset.")
    p.add_argument("--no-synth", action="store_true",
                   help="Skip synthesized stock basses; only render user WAVs from input/.")
    p.add_argument("--list-presets", action="store_true",
                   help="Print available presets and exit.")
    p.add_argument("--tag", default=None,
                   help="Optional suffix appended to output filenames (e.g. 'fast' → ..._fast.wav). "
                        "Useful when A/B-ing different overrides.")
    return p.parse_args(argv)


def select(items_by_name: dict, names: list | None, label: str) -> dict:
    if not names:
        return items_by_name
    missing = [n for n in names if n not in items_by_name]
    if missing:
        raise SystemExit(f"unknown {label}(s): {missing}. available: {sorted(items_by_name)}")
    return {n: items_by_name[n] for n in names}


def main(argv=None):
    args = parse_args(argv)
    root = Path(__file__).resolve().parent
    preset_dir = root / "presets"
    input_dir = root / "input"
    out_dir = root / "output"
    out_dir.mkdir(parents=True, exist_ok=True)

    all_presets = load_presets(preset_dir)
    if args.list_presets:
        for p in all_presets.values():
            print(f"  {p.name:30s} [{p.tier}]  {p.description}")
        return 0

    presets = select(all_presets, args.preset, "preset")
    sources = select(load_sources(input_dir, SAMPLE_RATE, include_synth=not args.no_synth),
                     args.source, "source")

    tag = f"_{args.tag}" if args.tag else ""

    for src_name, audio in sources.items():
        dry_path = out_dir / f"{src_name}__dry.wav"
        if not dry_path.exists():
            sf.write(str(dry_path), audio, SAMPLE_RATE)

        for preset_name, base_preset in presets.items():
            preset = Preset.from_dict({
                "name": base_preset.name,
                "description": base_preset.description,
                "tier": base_preset.tier,
                "cores": [{"id": c.id, "type": c.type,
                           **({"macros": dict(c.macros)} if c.macros else {}),
                           **c.params}
                          for c in base_preset.cores],
                "chain": [{"id": e.id, "type": e.type, **e.params} for e in base_preset.chain],
                "modulation": [{"src": m.src, "dst": m.dst, "depth": m.depth,
                                "scale": m.scale, "range": m.range} for m in base_preset.modulation],
                "output_gain_db": base_preset.output_gain_db,
                "seed": base_preset.seed,
            })
            if args.override:
                apply_overrides(preset, args.override)

            processed = run_preset(preset, audio, SAMPLE_RATE)
            out_path = out_dir / f"{src_name}__{preset_name}{tag}.wav"
            sf.write(str(out_path), processed, SAMPLE_RATE)
            print(f"  wrote {out_path.name}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
