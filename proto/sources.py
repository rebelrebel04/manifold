import numpy as np


def _env(n, sample_rate, attack_ms=5.0, release_ms=400.0):
    env = np.ones(n)
    a = int(attack_ms * sample_rate / 1000.0)
    r = int(release_ms * sample_rate / 1000.0)
    if a > 0:
        env[:a] = np.linspace(0.0, 1.0, a)
    if r > 0 and r < n:
        env[-r:] *= np.linspace(1.0, 0.0, r)
    return env


def synth_bass_saw(freq_hz, duration_s, sample_rate):
    # Naive saw + sub sine, light lowpass-ish via integrated noise. Bandlimiting ignored (prototype).
    n = int(duration_s * sample_rate)
    t = np.arange(n) / sample_rate
    saw = 2.0 * (t * freq_hz - np.floor(0.5 + t * freq_hz))
    sub = np.sin(2.0 * np.pi * (freq_hz * 0.5) * t)
    sig = 0.6 * saw + 0.4 * sub
    return (sig * _env(n, sample_rate)).astype(np.float32)


def synth_bass_fm(carrier_hz, duration_s, sample_rate, ratio=0.5, index=3.0):
    # 2-op FM: modulator at ratio*carrier, index sweeps down over the note.
    n = int(duration_s * sample_rate)
    t = np.arange(n) / sample_rate
    index_env = index * np.exp(-3.0 * t / duration_s)
    mod = np.sin(2.0 * np.pi * (carrier_hz * ratio) * t)
    car = np.sin(2.0 * np.pi * carrier_hz * t + index_env * mod)
    return (car * _env(n, sample_rate)).astype(np.float32)


def synth_bass_sub(freq_hz, duration_s, sample_rate):
    # Pure sine sub + soft saturation for a bit of character.
    n = int(duration_s * sample_rate)
    t = np.arange(n) / sample_rate
    sine = np.sin(2.0 * np.pi * freq_hz * t)
    sat = np.tanh(2.0 * sine)
    return (sat * _env(n, sample_rate)).astype(np.float32)
