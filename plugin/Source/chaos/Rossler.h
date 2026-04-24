#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::chaos
{

// Rössler attractor — ported from proto/chaos/rossler.py.
// In-plane spiraling builds tension, then z excursion reinjects. Slower-evolving,
// smoother orbital character than Lorenz's regime-switching.
class Rossler
{
public:
    struct Sample { double x, y, z; };

    void reset (double ix = 0.1, double iy = 0.0, double iz = 0.0) noexcept
    {
        state.x = ix; state.y = iy; state.z = iz;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setC     (double cv) noexcept { c = cv; }
    void setA     (double av) noexcept { a = av; }
    void setB     (double bv) noexcept { b = bv; }

    Sample step (double sampleRate) noexcept
    {
        // Rössler natural orbit period is ~6 time units vs Lorenz ~0.6 — scale dt so
        // speed=1.0 feels roughly comparable to Lorenz speed=1.0.
        const double dt = speed * 1.5 / sampleRate;
        const double dx = -state.y - state.z;
        const double dy = state.x + a * state.y;
        const double dz = b + state.z * (state.x - c);
        state.x += dx * dt;
        state.y += dy * dt;
        state.z += dz * dt;

        // Explicit Euler + z*(x-c) feedback can blow up under extreme params.
        // Re-seed if state diverges so the engine never produces NaN audio.
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

    // Widened ranges handle non-canonical c values where z excursions can get large.
    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 20.0,        -1.0, 1.0),
            std::clamp (s.y / 20.0,        -1.0, 1.0),
            std::clamp ((s.z - 15.0) / 20.0, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return state; }

private:
    Sample state { 0.1, 0.0, 0.0 };
    double speed = 1.0;
    double a     = 0.2;
    double b     = 0.2;
    double c     = 5.7;
};

} // namespace manifold::chaos
