import numpy as np


def wavefold(x, drive):
    # Triangle-wave (symmetric) wavefolder. drive may be scalar or per-sample array.
    y = x * drive
    y = np.mod(y + 1.0, 4.0) - 1.0
    return np.where(y > 1.0, 2.0 - y, y)
