import logging
import asyncio
import json
import time
import inspect


class WebSocketWrap:
    def __init__(self, ws, logger=None):
        if logger is None:
            logger = logging.getLogger("websocket")

        self._ws = ws
        self.logger = logger

    def close(self):
        self._ws.close()

    async def write(self, msg):
        if isinstance(msg, bytes):
            self.logger.debug(f"Send binary message: {msg}")
            ret = self._ws.write_message(msg, binary=True)
        else:
            self.logger.debug(f"Send JSON message: {msg}")
            ret = self._ws.write_message(json.dumps(msg), binary=False)

        if inspect.isawaitable(ret):
            return await ret
        return ret

    async def read(self, timeout=5):
        msg = await asyncio.wait_for(self._ws.read_message(), timeout)
        if msg is None:
            self.logger.debug("End-Of-Stream")
            return msg
        elif isinstance(msg, bytes):
            self.logger.debug(f"Received binary message: {msg}")
            return msg
        else:
            msg = json.loads(msg)
            self.logger.debug(f"Received JSON message: {msg}")
            return msg

    async def read_all(self, timeout=1):
        deadline = time.monotonic() + timeout
        msgs = []

        while True:
            msg_timeout = deadline - time.monotonic()
            if msg_timeout <= 0:
                raise asyncio.TimeoutError()

            msg = await self.read(timeout=msg_timeout)
            if msg is None:
                return msgs
            msgs.append(msg)

    async def read_some(self, timeout=1):
        msgs = []

        deadline = time.monotonic() + timeout
        while True:
            msg_timeout = deadline - time.monotonic()
            if msg_timeout <= 0:
                break
            try:
                msg = await self.read(timeout=msg_timeout)
                msgs.append(msg)
                if msg is None:
                    break
            except asyncio.TimeoutError:
                break

        return msgs
