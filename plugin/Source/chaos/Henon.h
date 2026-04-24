#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::chaos
{

// Hénon map — 2D discrete attractor: x_{n+1} = 1 - a*x^2 + y, y_{n+1} = b*x.
// Distinct from the ODE engines: updates are instantaneous (no dt integration), giving
// an angular, stepwise modulation feel with intermittent regime bursts.
//
// z is synthesized as the x-velocity (x_n - x_{n-1}) to fill the third mod axis.
// SPEED controls the map update rate (Hz); samples between updates are linearly
// interpolated so the output is audio-rate smooth despite the discrete steps.
class Henon
{
public:
    struct Sample { double x, y, z; };

    void reset() noexcept
    {
        cur  = { 0.1,  0.0, 0.0 };
        prev = { 0.0,  0.0, 0.0 };
        phase = 0.0;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setA     (double av) noexcept { a = av; }

    Sample step (double sampleRate) noexcept
    {
        // Map speed macro [0.3, 2.8] to update rate [5, 70] Hz.
        // Low speed = slow stepped wobble; high speed = rapid discrete bursts.
        const double updateHz = 5.0 + (70.0 - 5.0) * (speed - 0.3) / 2.5;
        phase += updateHz / sampleRate;

        if (phase >= 1.0)
        {
            phase -= 1.0;
            prev = cur;

            const double xNew = 1.0 - a * cur.x * cur.x + cur.y;
            const double yNew = b * cur.x;

            // Check for divergence (escape from attractor basin) and re-seed.
            constexpr double kSafeBound = 10.0;
            if (! std::isfinite (xNew) || ! std::isfinite (yNew)
                || std::abs (xNew) > kSafeBound || std::abs (yNew) > kSafeBound)
            {
                reset();
                return cur;
            }

            cur.z = xNew - cur.x;   // velocity as third mod axis
            cur.x = xNew;
            cur.y = yNew;
        }

        // Linearly interpolate between the last two map states for audio-rate smoothness.
        const double t = phase;
        return {
            prev.x + (cur.x - prev.x) * t,
            prev.y + (cur.y - prev.y) * t,
            prev.z + (cur.z - prev.z) * t,
        };
    }

    // x ~ [-1.33, 1.33], y ~ [-0.4, 0.4], z (velocity) ~ [-3.0, 3.0].
    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 1.5, -1.0, 1.0),
            std::clamp (s.y / 0.5, -1.0, 1.0),
            std::clamp (s.z / 3.0, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return cur; }

private:
    Sample cur  { 0.1, 0.0, 0.0 };
    Sample prev { 0.0, 0.0, 0.0 };
    double phase = 0.0;
    double speed = 1.0;
    double a     = 1.4;
    double b     = 0.3;
};

} // namespace manifold::chaos
