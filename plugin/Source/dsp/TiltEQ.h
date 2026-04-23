#pragma once

#include <cmath>

namespace manifold::dsp
{

// Baxandall-style tilt: splits the signal into low/high around a fixed pivot via
// a 1-pole LP, then recombines with opposite gains. W -> 1 rotates the spectrum
// darker (boost lows, cut highs) without touching the middle.
class TiltEQ
{
public:
    void reset() noexcept { lpState = 0.0f; }

    void prepare (float sampleRate, float pivotHz) noexcept
    {
        constexpr float kTwoPi = 6.2831853f;
        alpha = 1.0f - std::exp (-kTwoPi * pivotHz / sampleRate);
    }

    float process (float x, float gainLow, float gainHigh) noexcept
    {
        lpState += alpha * (x - lpState);
        const float high = x - lpState;
        return lpState * gainLow + high * gainHigh;
    }

private:
    float alpha   = 0.0f;
    float lpState = 0.0f;
};

} // namespace manifold::dsp
