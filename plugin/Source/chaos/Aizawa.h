#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::chaos
{

// Aizawa attractor — ported from proto/chaos/aizawa.py.
// Flower-like 3D geometry with multi-petal orbital structure. More "sculpted"
// look than Lorenz/Thomas/Rossler; tends to sit around a central axis.
class Aizawa
{
public:
    struct Sample { double x, y, z; };

    void reset (double ix = 0.1, double iy = 0.0, double iz = 0.0) noexcept
    {
        state.x = ix; state.y = iy; state.z = iz;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setA     (double av) noexcept { a = av; }

    Sample step (double sampleRate) noexcept
    {
        const double dt = speed * 3.0 / sampleRate;
        const double x = state.x, y = state.y, z = state.z;
        const double dx = (z - b) * x - d * y;
        const double dy = d * x + (z - b) * y;
        const double dz = c + a * z - (z * z * z) / 3.0 - (x * x + y * y) * (1.0 + e * z) + f * z * (x * x * x);
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

    // Canonical Aizawa bounds: x,y ~[-1.5, 1.5], z ~[-0.5, 2.0].
    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 1.6,         -1.0, 1.0),
            std::clamp (s.y / 1.6,         -1.0, 1.0),
            std::clamp ((s.z - 0.75) / 1.25, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return state; }

private:
    Sample state { 0.1, 0.0, 0.0 };
    double speed = 1.0;
    double a = 0.95;
    double b = 0.7;
    double c = 0.6;
    double d = 3.5;
    double e = 0.25;
    double f = 0.1;
};

} // namespace manifold::chaos
