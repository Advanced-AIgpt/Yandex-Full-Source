import time
import threading
from collections import deque
import asyncio
import logging
import grpc


class AsyncGrpcStreamCall:
    Logger = logging.getLogger("apphost_grpc_client.AsyncGrpcStreamCall")

    class _CANCEL:
        pass

    async def __aenter__(self):
        return self

    async def __aexit__(self, *args, **kargs):
        self.cancel()
        await self.read_all(nothrow=True)

    def _push_in(self, item):
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
            asyncio.run_coroutine_threadsafe(self._out.put(chunk), loop)

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
                self.Logger.debug("early cancelation")
                stream.cancel()
                push_response(None)
                return

            for chunk in stream:
                push_response(chunk)

            self.Logger.debug("received End-Of-Stream")
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

        self._out = asyncio.Queue()

        self._thread = threading.Thread(target=self._run, args=(getattr(stub, name), asyncio.get_running_loop()))
        self._thread.start()
        self._finished = False

    def write(self, chunk):
        self._push_in(chunk)

    def writes_done(self):
        self._push_in(None)

    def cancel(self):
        self._push_in(self._CANCEL)

    async def read(self, timeout=None):
        x = await asyncio.wait_for(self._out.get(), timeout=timeout)
        self._out.task_done()

        if isinstance(x, Exception):
            self._finished = True
            raise x

        if x is None:
            self._finished = True

        return x

    async def read_all(self, nothrow=False, chunk_timeout=None, timeout=None):
        chunks = []

        if self._finished:
            return chunks

        if timeout is not None:
            deadline = time.time() + timeout
        else:
            deadline = None

        try:
            while True:
                if deadline is not None:
                    current_time = time.time()
                    if current_time >= deadline:
                        raise asyncio.TimeoutError()

                    if chunk_timeout is None:
                        chunk_timeout = deadline - current_time
                    else:
                        chunk_timeout = min(chunk_timeout, deadline - current_time)

                chunk = await self.read(timeout=chunk_timeout)
                if chunk is None:
                    return chunks
                chunks.append(chunk)

        except:
            self.Logger.exception("failed to read")
            if nothrow:
                return chunks
            raise
