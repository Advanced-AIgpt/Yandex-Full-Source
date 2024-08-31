from .common import UniProxyProcess
import tornado.ioloop
import pytest


@pytest.fixture(scope="module")
def uniproxy():
    with UniProxyProcess() as x:
        yield x


def run_async(func):
    def test_async_wrap(uniproxy):
        error = None
        ioloop = tornado.ioloop.IOLoop.current()

        async def func_with_args():
            try:
                await func(uniproxy)
            except Exception as e:
                nonlocal error
                error = e
            ioloop.stop()

        ioloop.spawn_callback(func_with_args)
        ioloop.start()
        if error is not None:
            raise error

    return test_async_wrap
