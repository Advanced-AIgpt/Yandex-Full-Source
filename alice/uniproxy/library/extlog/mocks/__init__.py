import json
import logging
from alice.uniproxy.library.logging import Logger
from tornado.queues import Queue


class SessionLogMock:
    def __enter__(self):
        class Handler(logging.Handler):
            def emit(_, record: logging.LogRecord):
                rec = json.loads(record.getMessage())
                self.records.append(rec)
                self.queue.put_nowait(rec)

        Logger.init('unittest', True)

        logger = logging.getLogger("unittest.sessionlogger.mock")
        logger.setLevel(logging.INFO)
        logger.propagate = False
        logger.addHandler(Handler())

        self._orig_log = Logger._session_log
        Logger._session_log = logger

        return self

    def __exit__(self, exp_type, exp_value, traceback):
        Logger._session_log = self._orig_log
        self._orig_log = None

    def __init__(self):
        self.records = []
        self.queue = Queue()
        self._orig_log = None

    async def pop_record(self):
        msg = await self.queue.get()
        self.queue.task_done()
        return msg


class WebHandlerLoggerMock:
    def __enter__(self):
        Logger.get(".webhandler").addHandler(self._handler)
        return self

    def __exit__(self, exp_type, exp_value, traceback):
        Logger.get(".webhandler").removeHandler(self._handler)

    def __init__(self):
        class Handler(logging.Handler):
            def emit(_, record):
                msg = record.getMessage()
                self.queue.put_nowait(json.loads(msg[len("WEBHANDLER: "):]))

        self.queue = Queue()
        self._handler = Handler()

    async def pop_record(self):
        msg = await self.queue.get()
        self.queue.task_done()
        return msg
