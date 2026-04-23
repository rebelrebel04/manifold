from __future__ import annotations

# Per-core macro mappings. Each macro is in [0, 1].
# intensity → chaos-depth knob (period-doubling / orbit complexity)
# speed     → orbit rate (maps to existing `speed` param)
# warmth    → output smoothing (high warmth = low-pass the mod stream more heavily)
#
# Raw params in a preset override resolved macros — macros are defaults, not a cage.


def _lerp(a, b, t):
    return a + (b - a) * t


def _warmth_to_smoothing_hz(warmth):
    if warmth <= 0.0:
        return 0.0
    return _lerp(400.0, 8.0, min(warmth, 1.0))


_INTENSITY = {
    "lorenz":  ("rho",   22.0, 42.0),
    "rossler": ("c",      4.5, 14.0),
    "thomas":  ("b",     0.33, 0.05),   # inverted: high intensity → low b → chaotic
    "chua":    ("alpha", 12.0, 20.0),
    "aizawa":  ("d",      2.5,  5.0),
}

_SPEED_RANGE = {
    "lorenz":  (0.3, 2.8),
    "rossler": (0.3, 4.0),
    "thomas":  (0.3, 2.5),
    "chua":    (0.3, 2.5),
    "aizawa":  (0.3, 2.5),
}


def resolve_macros(core_type: str, macros: dict) -> dict:
    if not macros:
        return {}
    out = {}

    if "intensity" in macros and core_type in _INTENSITY:
        param, lo, hi = _INTENSITY[core_type]
        out[param] = _lerp(lo, hi, float(macros["intensity"]))

    if "speed" in macros and core_type in _SPEED_RANGE:
        lo, hi = _SPEED_RANGE[core_type]
        out["speed"] = _lerp(lo, hi, float(macros["speed"]))

    if "warmth" in macros:
        out["smoothing_hz"] = _warmth_to_smoothing_hz(float(macros["warmth"]))

    return out
