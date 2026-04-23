#pragma once

#include <algorithm>

namespace manifold::chaos
{

// Lorenz attractor — ported from proto/chaos/lorenz.py.
// Generates one normalized 3-axis sample per call to step().
class Lorenz
{
public:
    struct Sample { double x, y, z; };

    void reset (double ix = 1.0, double iy = 1.0, double iz = 1.0) noexcept
    {
        state.x = ix; state.y = iy; state.z = iz;
    }

    void setSpeed (double s) noexcept { speed = s; }
    void setRho   (double r) noexcept { rho = r; }
    void setSigma (double s) noexcept { sigma = s; }
    void setBeta  (double b) noexcept { beta = b; }

    // Returns next state in raw attractor units. Use normalize() to map to [-1, 1].
    Sample step (double sampleRate) noexcept
    {
        const double dt = speed * 5.0 / sampleRate;
        const double dx = sigma * (state.y - state.x);
        const double dy = state.x * (rho - state.z) - state.y;
        const double dz = state.x * state.y - beta * state.z;
        state.x += dx * dt;
        state.y += dy * dt;
        state.z += dz * dt;
        return state;
    }

    static Sample normalize (Sample s) noexcept
    {
        return {
            std::clamp (s.x / 20.0, -1.0, 1.0),
            std::clamp (s.y / 27.0, -1.0, 1.0),
            std::clamp ((s.z - 25.0) / 25.0, -1.0, 1.0),
        };
    }

    Sample current() const noexcept { return state; }

private:
    Sample state { 1.0, 1.0, 1.0 };
    double speed = 1.0;
    double sigma = 10.0;
    double rho   = 28.0;
    double beta  = 8.0 / 3.0;
};

} // namespace manifold::chaos
