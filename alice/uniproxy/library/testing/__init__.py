# import logging
import uuid

import tornado.gen
import tornado.ioloop
import tornado.stack_context


def uuidgen(binary=False):
    if binary:
        return uuid.uuid4().bytes
    else:
        return str(uuid.uuid4())


def ioloop_run(fn):
    # exception = None

    # logger = logging.getLogger('utils.testing')

    @tornado.gen.coroutine
    def async_wrapper(context, *args, **kwargs):
        try:
            yield tornado.gen.coroutine(fn)(*args, **kwargs)
        except Exception as ex:
            context['exception'] = ex
        except AssertionError as ex:
            context['exception'] = ex
        finally:
            tornado.ioloop.IOLoop.current().stop()

    def wrapper(*args, **kwargs):
        context = {}

        tornado.ioloop.IOLoop.current().spawn_callback(async_wrapper, context, *args, **kwargs)
        tornado.ioloop.IOLoop.current().start()

        if 'exception' in context:
            raise context['exception']

    return wrapper


def run_async(timeout=None):

    class TestIOLoop(tornado.ioloop.IOLoop.current().__class__):
        uncaught_exceptions = 0

        def handle_callback_exception(self, callback):
            self.__class__.uncaught_exceptions += 1
            super().handle_callback_exception(callback)

    def decorator(func):
        def test_async_wrap(*args, **kwargs):
            async def func_with_args():
                return await func(*args, **kwargs)
            TestIOLoop().run_sync(func_with_args, timeout=timeout)
            assert TestIOLoop.uncaught_exceptions == 0, f"There are {TestIOLoop.uncaught_exceptions} uncaught exceptions"

        return test_async_wrap

    return decorator
