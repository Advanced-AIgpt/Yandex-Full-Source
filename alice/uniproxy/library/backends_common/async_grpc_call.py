import grpc
import time

import threading

from tornado.ioloop import IOLoop
from tornado.queues import Queue

from alice.uniproxy.library.logging import Logger

from collections import deque


class AsyncGrpcStreamCall:
    class _CANCEL:
        pass

    async def __aenter__(self):
        return self

    async def __aexit__(self, *args, **kargs):
        self.cancel()
        await self.read_all(nothrow=True)

    def _push_in(self, item):
        # Blocks tornado ioloop thread
        # However it is fast enough
        with self._in_cv:
            self._in.append(item)
            self._in_cv.notify()

    def _pop_in(self):
        with self._in_cv:
            while len(self._in) == 0:
                self._in_cv.wait()
            return self._in.popleft()

    def _run(self, call, loop):
        stream = None
        cancelled = False

        def push_response(chunk):
            loop.add_callback(self._out.put, chunk)

        def request_iterator():
            nonlocal cancelled

            while True:
                chunk = self._pop_in()

                if chunk is None:
                    return

                if chunk is self._CANCEL:
                    cancelled = True
                    if stream is not None:
                        stream.cancel()
                    return

                yield chunk

        try:
            stream = call(request_iterator(), timeout=self._timeout)
            if cancelled:  # stream was cancelled before first outgoing chunk was produced
                self._log.debug("early cancelation")
                stream.cancel()
                push_response(None)
                return

            # WARNING: Thread is blocked here
            # Do not try to run it in tornado ioloop
            # There is a solution in gRPC: https://github.com/grpc/grpc/blob/fd3bd70939fb4239639fbd26143ec416366e4157/src/python/grpcio/grpc/aio/_base_call.py#L204-L244
            # However it is not synced to arcadia :(
            for chunk in stream:
                self._log.debug("received chunk")
                push_response(chunk)

            self._log.debug("received End-Of-Stream")
            push_response(None)

        except grpc.RpcError as exc:
            if exc.code() == grpc.StatusCode.CANCELLED:
                push_response(None)
            else:
                push_response(exc)

        except Exception as exc:
            push_response(exc)

    def __init__(self, stub, name, timeout=None):
        self._timeout = timeout

        self._in = deque()
        self._in_cv = threading.Condition()
        self._log = Logger.get('.backends.asyncgrpcstreamcall')

        self._out = Queue()

        self._thread = threading.Thread(
            target=self._run,
            args=(getattr(stub, name), IOLoop.current())
        )
        self._thread.start()
        self._finished = False

    def write(self, chunk):
        self._push_in(chunk)

    def writes_done(self):
        self._push_in(None)

    def cancel(self):
        self._push_in(self._CANCEL)

    async def read(self, timeout=None):
        if timeout is not None:
            deadline = time.time() + timeout
        else:
            deadline = None

        # Read note from https://www.tornadoweb.org/en/stable/queues.html#tornado.queues.Queue.get
        # Timeout means deadline here
        x = await self._out.get(timeout=deadline)
        self._out.task_done()

        if isinstance(x, Exception):
            self._finished = True
            raise x

        if x is None:
            self._finished = True

        return x

    async def read_all(self, nothrow=False):
        chunks = []

        if self._finished:
            return chunks

        try:
            while True:
                chunk = await self.read()
                if chunk is None:
                    return chunks
                chunks.append(chunk)

        except:
            self._log.exception("failed to read")
            if nothrow:
                return chunks
            raise
