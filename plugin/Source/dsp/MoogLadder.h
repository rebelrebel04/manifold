#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::dsp
{

// Four cascaded 1-pole LPs with a tanh-saturated feedback path.
// Self-oscillates near k = 4 (resonance ~ 1.0). Classic warm Moog character.
class MoogLadder
{
public:
    void reset() noexcept { y1 = y2 = y3 = y4 = 0.0f; }

    // morph blends 4-pole (0) -> 2-pole (1): classic-dark to brighter tonal slope.
    float process (float x, float cutoffHz, float resonance, float morph, float sampleRate) noexcept
    {
        const float fc = std::clamp (cutoffHz, 20.0f, sampleRate * 0.45f);
        const float T  = 1.0f / sampleRate;

        // TPT 1-pole coefficient — pre-warped tan() keeps cutoff accurate near Nyquist.
        const float wa = (2.0f / T) * std::tan (kPi * fc * T);
        const float g  = wa * T * 0.5f;
        const float G  = g / (1.0f + g);

        const float k = 4.0f * std::clamp (resonance, 0.0f, 0.99f);

        const float u = std::tanh (x - k * y4);

        y1 += G * (u  - y1);
        y2 += G * (y1 - y2);
        y3 += G * (y2 - y3);
        y4 += G * (y3 - y4);

        const float m = std::clamp (morph, 0.0f, 1.0f);
        const float out = y4 + (y2 - y4) * m;

        // Resonance compensation — naive ladders lose level as k climbs.
        const float makeup = 1.0f + 0.5f * resonance;
        return out * makeup;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;
    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f, y4 = 0.0f;
};

} // namespace manifold::dsp
