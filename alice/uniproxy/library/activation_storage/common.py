import math
import random
import time


def _make_random(seed=None):
    if seed is not None:
        return (hash(seed) % 1000) / 1000
    else:
        return random.random()


def make_decision(prob, seed=None):
    return (not math.isclose(prob, 0.0) and (math.isclose(prob, 1.0) or _make_random(seed) < prob))


def milliseconds() -> int:
    """ returns current timestamp in milliseconds """
    return int(time.time() * 1000)


def is_equal_rms(lhs, rhs):
    return math.isclose(lhs, 0.0) or math.isclose(rhs, 0.0) or math.isclose(lhs, rhs)


def compare_spotters_raw(
    l_validated: bool, l_rms: float, l_time: int, l_device_id: str,
    r_validated: bool, r_rms: float, r_time: int, r_device_id: str,
):
    """ returns True iff `r` is better"""
    if l_validated == r_validated:
        if is_equal_rms(l_rms, r_rms):
            return (r_time, r_device_id) < (l_time, l_device_id)
        return l_rms < r_rms
    return l_validated < r_validated
