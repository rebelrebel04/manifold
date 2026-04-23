#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::dsp
{

// TB-303-flavored ladder: asymmetric soft clip in the feedback path, tighter
// resonance peak that "screams" near self-oscillation. Coarser/edgier than Moog.
class DiodeLadder
{
public:
    void reset() noexcept { y1 = y2 = y3 = y4 = 0.0f; }

    // morph blends 4-pole (0) -> 2-pole (1): fat/dark to cutting/nasal.
    float process (float x, float cutoffHz, float resonance, float morph, float sampleRate) noexcept
    {
        const float fc = std::clamp (cutoffHz, 20.0f, sampleRate * 0.45f);
        const float T  = 1.0f / sampleRate;
        const float wa = (2.0f / T) * std::tan (kPi * fc * T);
        const float g  = wa * T * 0.5f;
        const float G  = g / (1.0f + g);

        // Diode topology takes higher feedback gain to self-oscillate.
        const float k = 6.5f * std::clamp (resonance, 0.0f, 0.99f);

        // Asymmetric diode-style softclip: y = v / (1 + |v|^1.5) — pushes 2nd harmonic.
        auto diode = [] (float v) noexcept
        {
            const float a = std::abs (v);
            return v / (1.0f + a * std::sqrt (a));
        };

        const float u = diode (x - k * y4);

        y1 += G * (u  - y1);
        y2 += G * (y1 - y2);
        y3 += G * (y2 - y3);
        y4 += G * (y3 - y4);

        const float m = std::clamp (morph, 0.0f, 1.0f);
        const float out = y4 + (y2 - y4) * m;

        // 303 ducks the through-signal and amplifies the resonance — fake it
        // by boosting output as resonance climbs.
        const float makeup = 1.0f + 1.2f * resonance;
        return out * makeup;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;
    float y1 = 0.0f, y2 = 0.0f, y3 = 0.0f, y4 = 0.0f;
};

} // namespace manifold::dsp
