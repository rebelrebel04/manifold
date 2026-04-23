import numpy as np


def svf(x, cutoff, resonance, sample_rate):
    # Chamberlin state-variable filter. cutoff and resonance may be scalars or per-sample arrays.
    # Returns (lp, bp, hp). Resonance in [0, 1) -> Q = 1/(1-res), clamped to sane range.

    n = len(x)
    cutoff_arr = np.broadcast_to(np.asarray(cutoff, dtype=np.float64), (n,))
    res_arr = np.broadcast_to(np.asarray(resonance, dtype=np.float64), (n,))

    # Chamberlin stability limit: f < 2*sin(pi/3). Clamp cutoff to < sample_rate/3.
    cutoff_arr = np.clip(cutoff_arr, 20.0, sample_rate * 0.3)
    f = 2.0 * np.sin(np.pi * cutoff_arr / sample_rate)
    q = np.clip(1.0 - res_arr, 0.02, 2.0)

    lp = np.empty(n)
    bp = np.empty(n)
    hp = np.empty(n)
    lp_z = 0.0
    bp_z = 0.0
    for i in range(n):
        hp_i = x[i] - lp_z - q[i] * bp_z
        bp_i = bp_z + f[i] * hp_i
        lp_i = lp_z + f[i] * bp_i
        hp[i] = hp_i
        bp[i] = bp_i
        lp[i] = lp_i
        lp_z = lp_i
        bp_z = bp_i
    return lp, bp, hp


def svf_morph(x, cutoff, resonance, morph, sample_rate):
    # morph in [0, 1]: 0 = LP, 0.5 = BP, 1 = HP. Smooth crossfade.
    lp, bp, hp = svf(x, cutoff, resonance, sample_rate)
    m = np.broadcast_to(np.asarray(morph, dtype=np.float64), (len(x),))
    # Triangle weights so each mode peaks once.
    w_lp = np.clip(1.0 - 2.0 * m, 0.0, 1.0)
    w_hp = np.clip(2.0 * m - 1.0, 0.0, 1.0)
    w_bp = 1.0 - w_lp - w_hp
    return w_lp * lp + w_bp * bp + w_hp * hp
