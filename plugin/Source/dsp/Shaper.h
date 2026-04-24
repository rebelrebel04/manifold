#pragma once

#include <algorithm>
#include <cmath>
#include "Wavefolder.h"

namespace manifold::dsp::shaper
{

enum class Type : int
{
    Fold = 0,
    SoftClip,
    HardClip,
    Rectify,
    Sine,
    TubeAsym,
    ChebyT3,
    ChebyT5,
};

// Triangle-fold (existing wavefolder).
inline float fold (float x, float drive) noexcept
{
    return manifold::dsp::wavefold (x, drive);
}

// tanh saturator. Increases odd-harmonic content as drive grows.
inline float softClip (float x, float drive) noexcept
{
    return std::tanh (x * drive);
}

// Brick-wall clip. Sharp-edged, aggressive harmonic content.
inline float hardClip (float x, float drive) noexcept
{
    return std::clamp (x * drive, -1.0f, 1.0f);
}

// Full-wave rectify, centered around 0. Octave-up character.
inline float rectify (float x, float drive) noexcept
{
    const float r = 2.0f * std::abs (x * drive) - 1.0f;
    return std::clamp (r, -1.0f, 1.0f);
}

// Sine shaper — drive > 1 wraps the curve, producing fold-like harmonics but smoother.
inline float sine (float x, float drive) noexcept
{
    constexpr float kHalfPi = 1.57079633f;
    return std::sin (x * drive * kHalfPi);
}

// Asymmetric tube-style saturation. Adds even (2nd) harmonic content alongside odd.
inline float tubeAsym (float x, float drive) noexcept
{
    const float y = x * drive;
    return std::tanh (y + 0.3f * y * std::abs (y));
}

// Chebyshev T3: 4x^3 - 3x. Fed a pure sine, produces a pure 3rd harmonic.
// Arbitrary input gets a distinct "cubic" coloration.
inline float chebyT3 (float x, float drive) noexcept
{
    const float y = std::clamp (x * drive, -1.0f, 1.0f);
    return 4.0f * y * y * y - 3.0f * y;
}

// Chebyshev T5: 16x^5 - 20x^3 + 5x. Pure 5th harmonic on a sine; angular on anything else.
inline float chebyT5 (float x, float drive) noexcept
{
    const float y = std::clamp (x * drive, -1.0f, 1.0f);
    const float y2 = y * y;
    const float y3 = y2 * y;
    const float y5 = y3 * y2;
    return 16.0f * y5 - 20.0f * y3 + 5.0f * y;
}

inline float process (Type t, float x, float drive) noexcept
{
    switch (t)
    {
        case Type::SoftClip: return softClip (x, drive);
        case Type::HardClip: return hardClip (x, drive);
        case Type::Rectify:  return rectify  (x, drive);
        case Type::Sine:     return sine     (x, drive);
        case Type::TubeAsym: return tubeAsym (x, drive);
        case Type::ChebyT3:  return chebyT3  (x, drive);
        case Type::ChebyT5:  return chebyT5  (x, drive);
        case Type::Fold:
        default:             return fold     (x, drive);
    }
}

} // namespace manifold::dsp::shaper
