import numpy as np


class Chua:
    # Chua's circuit - 3D continuous double-scroll attractor.
    # Similar wing-switching character as Lorenz but with richer harmonic content in orbits.
    # Classic chaotic params: alpha=15.6, beta=28, m0=-1.143, m1=-0.714.

    def __init__(self, alpha=15.6, beta=28.0, m0=-1.143, m1=-0.714,
                 speed=1.0, init=(0.7, 0.0, 0.0)):
        self.alpha = alpha
        self.beta = beta
        self.m0 = m0
        self.m1 = m1
        self.speed = speed
        self.state = np.array(init, dtype=np.float64)

    def generate(self, n_samples, sample_rate):
        # Chua's natural timescale is fast; orbits ~0.3 natural time units.
        dt = self.speed * 10.0 / sample_rate
        out = np.empty((n_samples, 3))
        x, y, z = self.state
        alpha, beta, m0, m1 = self.alpha, self.beta, self.m0, self.m1
        for i in range(n_samples):
            # Piecewise-linear Chua diode: f(x) = m1*x + 0.5*(m0-m1)*(|x+1| - |x-1|)
            fx = m1 * x + 0.5 * (m0 - m1) * (abs(x + 1.0) - abs(x - 1.0))
            dx = alpha * (y - x - fx)
            dy = x - y + z
            dz = -beta * y
            x += dx * dt
            y += dy * dt
            z += dz * dt
            out[i, 0] = x
            out[i, 1] = y
            out[i, 2] = z
        self.state = np.array([x, y, z])
        return out

    @staticmethod
    def normalize(xyz):
        # Canonical Chua bounds: x~[-2.5,2.5], y~[-0.5,0.5], z~[-3.5,3.5].
        norm = np.empty_like(xyz)
        norm[:, 0] = np.clip(xyz[:, 0] / 3.0, -1.0, 1.0)
        norm[:, 1] = np.clip(xyz[:, 1] / 0.6, -1.0, 1.0)
        norm[:, 2] = np.clip(xyz[:, 2] / 4.0, -1.0, 1.0)
        return norm
