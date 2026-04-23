import numpy as np


class Duffing:
    # Driven Duffing oscillator: x'' + delta*x' + alpha*x + beta*x^3 = gamma*cos(omega*t) + input_drive*input(t)
    # Classic chaotic regime: delta=0.3, alpha=-1, beta=1, gamma=0.5, omega=1.2

    def __init__(self, delta=0.3, alpha=-1.0, beta=1.0, gamma=0.5, omega=1.2,
                 speed=1.0, init=(0.1, 0.0)):
        self.delta = delta
        self.alpha = alpha
        self.beta = beta
        self.gamma = gamma
        self.omega = omega
        self.speed = speed
        self.x, self.v = init
        self.t = 0.0

    def generate(self, n_samples, sample_rate, input_signal=None, input_drive=0.0):
        dt = self.speed * 3.0 / sample_rate
        out = np.empty((n_samples, 2))
        x, v, t = self.x, self.v, self.t
        delta, alpha, beta, gamma, omega = self.delta, self.alpha, self.beta, self.gamma, self.omega
        has_input = input_signal is not None and input_drive != 0.0
        for i in range(n_samples):
            force = gamma * np.cos(omega * t)
            if has_input:
                force += input_drive * input_signal[i]
            dv = -delta * v - alpha * x - beta * x * x * x + force
            x += v * dt
            v += dv * dt
            t += dt
            out[i, 0] = x
            out[i, 1] = v
        self.x, self.v, self.t = x, v, t
        return out

    @staticmethod
    def normalize(xv):
        norm = np.empty_like(xv)
        norm[:, 0] = np.clip(xv[:, 0] / 1.5, -1.0, 1.0)
        norm[:, 1] = np.clip(xv[:, 1] / 1.5, -1.0, 1.0)
        return norm
