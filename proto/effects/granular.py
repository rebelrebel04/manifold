import numpy as np


def granular(input_signal, sample_rate, grain_size_ms=60.0, overlap=3,
             position_mod=None, pitch_mod=None, jitter=0.0):
    # Simple overlap-add granular processor.
    # position_mod: array, values in [0, 1], sets grain read position (fraction of input length).
    # pitch_mod: array, pitch ratio (1.0 = no shift, 2.0 = octave up, 0.5 = octave down).
    # jitter: [0, 1], random offset added to read position.

    n = len(input_signal)
    grain_samples = int(grain_size_ms * sample_rate / 1000.0)
    hop = max(1, grain_samples // overlap)
    window = np.hanning(grain_samples).astype(np.float64)

    output = np.zeros(n + grain_samples)
    rng = np.random.default_rng(0)

    write_pos = 0
    while write_pos < n:
        pos = 0.5 if position_mod is None else float(position_mod[min(write_pos, n - 1)])
        pos = np.clip(pos + jitter * (rng.random() - 0.5), 0.0, 1.0)

        pitch = 1.0 if pitch_mod is None else float(pitch_mod[min(write_pos, n - 1)])
        pitch = max(0.25, min(4.0, pitch))

        # Source length needed to produce grain_samples at this pitch.
        src_len = int(np.ceil(grain_samples * pitch))
        max_start = max(0, n - src_len)
        start = int(pos * max_start)
        src = input_signal[start:start + src_len]
        if len(src) < src_len:
            src = np.pad(src, (0, src_len - len(src)))

        # Resample via linear interpolation to grain_samples.
        if src_len != grain_samples:
            grain = np.interp(
                np.linspace(0, src_len - 1, grain_samples),
                np.arange(src_len),
                src,
            )
        else:
            grain = src.astype(np.float64)

        grain = grain * window
        output[write_pos:write_pos + grain_samples] += grain
        write_pos += hop

    # Overlap=3 with Hann summing gives ~1.5x gain. Normalize.
    return (output[:n] / (overlap * 0.5)).astype(np.float32)
