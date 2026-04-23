#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace manifold::dsp
{

// Karplus-Strong / tuned comb resonator: delay line with a 1-pole LP in the
// feedback path. "cutoff" repurposes as pitch (Hz) — chaos modulating it gives
// unstable-pitch sustained resonance. "morph" sets the LP tone (decay brightness).
class TunedComb
{
public:
    void prepare (double sampleRate)
    {
        const int maxLen = (int) std::ceil (sampleRate / kMinFreqHz) + 8;
        buffer.assign ((size_t) maxLen, 0.0f);
        writePos = 0;
        lpState  = 0.0f;
    }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        lpState  = 0.0f;
        writePos = 0;
    }

    float process (float x, float pitchHz, float feedback, float toneHz, float sampleRate) noexcept
    {
        if (buffer.empty()) return x;

        const int   N    = (int) buffer.size();
        const float pHz  = std::clamp (pitchHz, kMinFreqHz, sampleRate * 0.45f);
        float       d    = sampleRate / pHz;
        d = std::clamp (d, 2.0f, (float) (N - 2));

        // Fractional read with linear interpolation — fine for an MVP.
        float r = (float) writePos - d;
        while (r < 0.0f) r += (float) N;
        const int   i0   = ((int) r) % N;
        const int   i1   = (i0 + 1) % N;
        const float frac = r - std::floor (r);
        const float y    = buffer[(size_t) i0]
                         + frac * (buffer[(size_t) i1] - buffer[(size_t) i0]);

        // 1-pole LP in feedback. toneHz dialed via the morph macro.
        const float tHz = std::clamp (toneHz, 80.0f, sampleRate * 0.45f);
        const float a   = 1.0f - std::exp (-kTwoPi * tHz / sampleRate);
        lpState += a * (y - lpState);

        const float fb = std::clamp (feedback, 0.0f, 0.999f);
        buffer[(size_t) writePos] = x + fb * lpState;
        writePos = (writePos + 1) % N;

        return y;
    }

private:
    static constexpr float kMinFreqHz = 20.0f;
    static constexpr float kTwoPi     = 6.28318530717958647692f;
    std::vector<float> buffer;
    int   writePos = 0;
    float lpState  = 0.0f;
};

} // namespace manifold::dsp
