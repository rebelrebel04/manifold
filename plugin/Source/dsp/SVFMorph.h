#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::dsp
{

// Chamberlin state-variable filter with LP↔BP↔HP morph crossfade.
// Per-channel state. Ported from proto/effects/svf.py (svf_morph).
class SVFMorph
{
public:
    void reset() noexcept { lpZ = 0.0f; bpZ = 0.0f; }

    // morph: 0 = LP, 0.5 = BP, 1 = HP. Triangle weights so each mode peaks once.
    float process (float x, float cutoff, float resonance, float morph, float sampleRate) noexcept
    {
        cutoff = std::clamp (cutoff, 20.0f, sampleRate * 0.3f);
        const float f = 2.0f * std::sin (kPi * cutoff / sampleRate);
        const float q = std::clamp (1.0f - resonance, 0.02f, 2.0f);

        const float hp = x - lpZ - q * bpZ;
        const float bp = bpZ + f * hp;
        const float lp = lpZ + f * bp;
        lpZ = lp;
        bpZ = bp;

        const float wLP = std::clamp (1.0f - 2.0f * morph, 0.0f, 1.0f);
        const float wHP = std::clamp (2.0f * morph - 1.0f, 0.0f, 1.0f);
        const float wBP = 1.0f - wLP - wHP;
        return wLP * lp + wBP * bp + wHP * hp;
    }

private:
    static constexpr float kPi = 3.14159265358979323846f;
    float lpZ = 0.0f;
    float bpZ = 0.0f;
};

} // namespace manifold::dsp
