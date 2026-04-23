import numpy as np


class Rossler:
    # 3D continuous chaos. Classic params a=0.2, b=0.2, c=5.7.
    # Orbital character: spiraling build-up in xy plane, then "reinjection" via z excursion.
    # Smoother than Lorenz — less regime-switching, more sustained tension cycles.

    def __init__(self, a=0.2, b=0.2, c=5.7, speed=1.0, init=(0.1, 0.0, 0.0)):
        self.a = a
        self.b = b
        self.c = c
        self.speed = speed
        self.state = np.array(init, dtype=np.float64)

    def generate(self, n_samples, sample_rate):
        # Rossler orbit period is ~6 natural time units vs Lorenz ~0.6, so scale dt so
        # speed=1.0 gives roughly comparable musical rate to Lorenz speed=1.0.
        dt = self.speed * 1.5 / sample_rate
        out = np.empty((n_samples, 3))
        x, y, z = self.state
        a, b, c = self.a, self.b, self.c
        for i in range(n_samples):
            dx = -y - z
            dy = x + a * y
            dz = b + z * (x - c)
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
        # Widened to accommodate non-canonical c values (up to ~15) where z excursions
        # get very large. At canonical c=5.7 the output uses ~60% of [-1,1] range.
        norm = np.empty_like(xyz)
        norm[:, 0] = np.clip(xyz[:, 0] / 20.0, -1.0, 1.0)
        norm[:, 1] = np.clip(xyz[:, 1] / 20.0, -1.0, 1.0)
        norm[:, 2] = np.clip((xyz[:, 2] - 15.0) / 20.0, -1.0, 1.0)
        return norm
