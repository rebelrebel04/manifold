import numpy as np


class Logistic:
    # Discrete 1D map: x_{n+1} = r * x_n * (1 - x_n).
    # r sweeps: 2.8 (stable) -> 3.2 (period-2) -> 3.5 (period-4) -> 3.57+ (chaos).
    # Emits one new value at `step_rate` Hz, linearly interpolated to sample_rate.

    def __init__(self, r=3.9, step_rate=80.0, x0=0.5):
        self.r = r
        self.step_rate = step_rate
        self.x = x0

    def generate(self, n_samples, sample_rate):
        n_steps = int(np.ceil(n_samples * self.step_rate / sample_rate)) + 2
        seq = np.empty(n_steps)
        x = self.x
        r = self.r
        for i in range(n_steps):
            x = r * x * (1.0 - x)
            seq[i] = x
        self.x = x

        t_audio = np.arange(n_samples) / sample_rate
        t_steps = np.arange(n_steps) / self.step_rate
        return np.interp(t_audio, t_steps, seq)

    @staticmethod
    def normalize(x):
        # Logistic output is in [0, 1]; map to [-1, 1] bipolar.
        return np.clip(x * 2.0 - 1.0, -1.0, 1.0)
