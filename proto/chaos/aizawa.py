import numpy as np


class Aizawa:
    # Aizawa attractor - flower-like 3D geometry with multi-petal orbital structure.
    # Classic params: a=0.95, b=0.7, c=0.6, d=3.5, e=0.25, f=0.1.

    def __init__(self, a=0.95, b=0.7, c=0.6, d=3.5, e=0.25, f=0.1,
                 speed=1.0, init=(0.1, 0.0, 0.0)):
        self.a = a
        self.b = b
        self.c = c
        self.d = d
        self.e = e
        self.f = f
        self.speed = speed
        self.state = np.array(init, dtype=np.float64)

    def generate(self, n_samples, sample_rate):
        dt = self.speed * 3.0 / sample_rate
        out = np.empty((n_samples, 3))
        x, y, z = self.state
        a, b, c, d, e, f = self.a, self.b, self.c, self.d, self.e, self.f
        for i in range(n_samples):
            dx = (z - b) * x - d * y
            dy = d * x + (z - b) * y
            dz = c + a * z - (z * z * z) / 3.0 - (x * x + y * y) * (1.0 + e * z) + f * z * (x * x * x)
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
        # Bounded roughly: x,y ~[-1.5, 1.5], z ~[-0.5, 2.0].
        norm = np.empty_like(xyz)
        norm[:, 0] = np.clip(xyz[:, 0] / 1.6, -1.0, 1.0)
        norm[:, 1] = np.clip(xyz[:, 1] / 1.6, -1.0, 1.0)
        norm[:, 2] = np.clip((xyz[:, 2] - 0.75) / 1.25, -1.0, 1.0)
        return norm
