import numpy as np


class Thomas:
    # Thomas' cyclically symmetric attractor. All three axes are identical in structure,
    # giving a very different orbital "feel" than Lorenz — no dominant plane, drifting 3D motion.
    # b parameter: b < ~0.208 → chaotic. b=0.208186 is the classic setting.

    def __init__(self, b=0.208186, speed=1.0, init=(0.1, 0.0, 0.0)):
        self.b = b
        self.speed = speed
        self.state = np.array(init, dtype=np.float64)

    def generate(self, n_samples, sample_rate):
        # Thomas orbits are slow in natural time; scale dt generously.
        dt = self.speed * 20.0 / sample_rate
        out = np.empty((n_samples, 3))
        x, y, z = self.state
        b = self.b
        for i in range(n_samples):
            dx = np.sin(y) - b * x
            dy = np.sin(z) - b * y
            dz = np.sin(x) - b * z
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
        # Bounded roughly in [-4, 4] per axis. Symmetric around 0.
        return np.clip(xyz / 4.0, -1.0, 1.0)
