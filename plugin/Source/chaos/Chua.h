#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::chaos
{

// Chua's circuit — ported from proto/chaos/chua.py.
// Double-scroll attractor: wing-switching like Lorenz but with richer harmonic
// content in orbits, driven by a piecewise-linear "diode" nonlinearity.
class Chua
{
public:
    struct Sample { double x, y, z; };

    void reset (double ix = 0.7, double iy = 0.0, double iz = 0.0) noexcept
    {
        state.x = ix; state.y = iy; state.z = iz;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setAlpha (double a) noexcept { alpha = a; }
    void setBeta  (double b) noexcept { beta  = b; }

    Sample step (double sampleRate) noexcept
    {
        // Chua's natural timescale is fast (~0.3 time units per orbit); 10x dt scale.
        const double dt = speed * 10.0 / sampleRate;

        // Piecewise-linear Chua diode: f(x) = m1*x + 0.5*(m0 - m1)*(|x+1| - |x-1|).
        const double fx = m1 * state.x + 0.5 * (m0 - m1) * (std::abs (state.x + 1.0) - std::abs (state.x - 1.0));
        const double dx = alpha * (state.y - state.x - fx);
        const double dy = state.x - state.y + state.z;
        const double dz = -beta * state.y;

        state.x += dx * dt;
        state.y += dy * dt;
        state.z += dz * dt;

        constexpr double kSafeBound = 200.0;
        if (! std::isfinite (state.x) || ! std::isfinite (state.y) || ! std::isfinite (state.z)
            || std::abs (state.x) > kSafeBound
            || std::abs (state.y) > kSafeBound
            || std::abs (state.z) > kSafeBound)
        {
            reset();
        }
        return state;
    }

    // Canonical Chua bounds: x~[-2.5,2.5], y~[-0.5,0.5], z~[-3.5,3.5].
    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 3.0, -1.0, 1.0),
            std::clamp (s.y / 0.6, -1.0, 1.0),
            std::clamp (s.z / 4.0, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return state; }

private:
    Sample state { 0.7, 0.0, 0.0 };
    double speed = 1.0;
    double alpha = 15.6;
    double beta  = 28.0;
    double m0    = -1.143;
    double m1    = -0.714;
};

} // namespace manifold::chaos
