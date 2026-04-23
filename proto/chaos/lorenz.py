import numpy as np


class Lorenz:
    def __init__(self, sigma=10.0, rho=28.0, beta=8.0 / 3.0, speed=1.0, init=(1.0, 1.0, 1.0)):
        self.sigma = sigma
        self.rho = rho
        self.beta = beta
        self.speed = speed
        self.state = np.array(init, dtype=np.float64)

    def generate(self, n_samples, sample_rate):
        dt = self.speed * 5.0 / sample_rate
        out = np.empty((n_samples, 3))
        x, y, z = self.state
        sigma, rho, beta = self.sigma, self.rho, self.beta
        for i in range(n_samples):
            dx = sigma * (y - x)
            dy = x * (rho - z) - y
            dz = x * y - beta * z
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
        norm = np.empty_like(xyz)
        norm[:, 0] = np.clip(xyz[:, 0] / 20.0, -1.0, 1.0)
        norm[:, 1] = np.clip(xyz[:, 1] / 27.0, -1.0, 1.0)
        norm[:, 2] = np.clip((xyz[:, 2] - 25.0) / 25.0, -1.0, 1.0)
        return norm
