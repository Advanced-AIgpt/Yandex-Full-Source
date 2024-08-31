import alice.uniproxy.library.tornado_speedups as speedups
import tornado.util as original
import time


def perftest(func):
    COUNT = 10000
    r = range(COUNT)

    def wrap(*args, **kwargs):
        t0 = time.monotonic_ns()
        for _ in r:
            func(*args, **kwargs)
        t1 = time.monotonic_ns()
        return (t1 - t0)/COUNT

    return wrap


def test_pyx_websocket_mask():
    data = b"1234567890"
    mask = b"ABCD"

    unmasked = speedups.pyx_websocket_mask(mask, data)
    assert unmasked == b"ppppttt|xr"


def test_perf():
    mask = b"ABCD"
    data_short = b"0123456789"
    data_medium = b"0123456789" * 10
    data_long = b"0123456789" * 100

    @perftest
    def original_mask(d):
        original._websocket_mask_python(mask, d)

    @perftest
    def speeduped_mask(d):
        speedups.pyx_websocket_mask(mask, d)

    for _ in range(3):
        print("--- ORIGINAL SHORT: ", original_mask(data_short))
        print("--- SPEEDUPED SHORT: ", speeduped_mask(data_short))
        print("--- ORIGINAL MEDIUM: ", original_mask(data_medium))
        print("--- SPEEDUPED MEDIUM: ", speeduped_mask(data_medium))
        print("--- ORIGINAL LONG: ", original_mask(data_long))
        print("--- SPEEDUPED LONG: ", speeduped_mask(data_long))
