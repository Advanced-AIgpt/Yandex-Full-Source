from tornado.concurrent import Future, chain_future
from tornado.gen import convert_yielded
from tornado.ioloop import IOLoop

import logging


def with_result_on_timeout(timeout, future, timeout_result, io_loop=None, quiet_exceptions=()):
    """Wraps a `.Future` (or other yieldable object) in a timeout.

    return timeout_result if the input future does not complete before
    ``timeout``, which may be specified in any form allowed by
    `.IOLoop.add_timeout` (i.e. a `datetime.timedelta` or an absolute time
    relative to `.IOLoop.time`)

    If the wrapped `.Future` fails after it has timed out, the exception
    will be logged unless it is of a type contained in ``quiet_exceptions``
    (which may be an exception type or a sequence of types).

    Does not support `YieldPoint` subclasses.
    """
    # It's tempting to optimize this by cancelling the input future on timeout
    # instead of creating a new one, but A) we can't know if we are the only
    # one waiting on the input future, so cancelling it might disrupt other
    # callers and B) concurrent futures can only be cancelled while they are
    # in the queue, so cancellation cannot reliably bound our waiting time.
    future = convert_yielded(future)
    result = Future()
    chain_future(future, result)
    if io_loop is None:
        io_loop = IOLoop.current()

    def error_callback(future):
        try:
            future.result()
        except Exception as e:
            if not isinstance(e, quiet_exceptions):
                log = logging.getLogger('uniproxy')
                log.error("Exception in Future %r after timeout", future, exc_info=True)

    def timeout_callback():
        if not result.done():
            result.set_result(timeout_result)
        # In case the wrapped future goes on to fail, log it.
        future.add_done_callback(error_callback)
    timeout_handle = io_loop.add_timeout(
        timeout, timeout_callback)
    if isinstance(future, Future):
        # We know this future will resolve on the IOLoop, so we don't
        # need the extra thread-safety of IOLoop.add_future (and we also
        # don't care about StackContext here.
        future.add_done_callback(
            lambda future: io_loop.remove_timeout(timeout_handle))
    else:
        # concurrent.futures.Futures may resolve on any thread, so we
        # need to route them back to the IOLoop.
        io_loop.add_future(
            future, lambda future: io_loop.remove_timeout(timeout_handle))
    return result
