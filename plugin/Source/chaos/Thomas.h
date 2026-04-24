#pragma once

#include <algorithm>
#include <cmath>

namespace manifold::chaos
{

// Thomas' cyclically symmetric attractor — ported from proto/chaos/thomas.py.
// All three axes are structurally identical; gives a braided 3D motion with no
// dominant plane, unlike Lorenz's two-lobed butterfly.
//
// b is the damping coefficient. Chaotic for b < ~0.208; lower b = more ergodic.
class Thomas
{
public:
    struct Sample { double x, y, z; };

    void reset (double ix = 0.1, double iy = 0.0, double iz = 0.0) noexcept
    {
        state.x = ix; state.y = iy; state.z = iz;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setB     (double bv) noexcept { b = bv; }

    Sample step (double sampleRate) noexcept
    {
        // Thomas orbits are slow in natural time; matches the 20x dt scale in the proto.
        const double dt = speed * 20.0 / sampleRate;
        const double dx = std::sin (state.y) - b * state.x;
        const double dy = std::sin (state.z) - b * state.y;
        const double dz = std::sin (state.x) - b * state.z;
        state.x += dx * dt;
        state.y += dy * dt;
        state.z += dz * dt;

        if (! std::isfinite (state.x) || ! std::isfinite (state.y) || ! std::isfinite (state.z))
            reset();
        return state;
    }

    // Raw output roughly bounded in [-4, 4], symmetric around 0.
    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 4.0, -1.0, 1.0),
            std::clamp (s.y / 4.0, -1.0, 1.0),
            std::clamp (s.z / 4.0, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return state; }

private:
    Sample state { 0.1, 0.0, 0.0 };
    double speed = 1.0;
    double b     = 0.208186;
};

} // namespace manifold::chaos
