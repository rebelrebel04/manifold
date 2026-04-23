#pragma once

#include <cmath>

namespace manifold::dsp
{

// Triangle-wave (symmetric) wavefolder. Stateless. Ported from proto/effects/wavefolder.py.
inline float wavefold (float x, float drive) noexcept
{
    float y = x * drive;
    y = std::fmod (y + 1.0f, 4.0f);
    if (y < 0.0f) y += 4.0f;       // C fmod can return negatives; numpy's mod is positive.
    y -= 1.0f;
    return (y > 1.0f) ? (2.0f - y) : y;
}

} // namespace manifold::dsp
