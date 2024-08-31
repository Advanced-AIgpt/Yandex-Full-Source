import logging
import collections


class LogCollector:
    def __enter__(self):
        self._logger.addHandler(self._handler)
        return self

    def __exit__(self, exp_type, exp_value, traceback):
        self._logger.removeHandler(self._handler)

    def __init__(self, logger):
        class Handler(logging.Handler):
            def emit(_, record: logging.LogRecord):
                self.records.append(record.getMessage())

        self._logger = logger
        self._handler = Handler()
        self.records = collections.deque()

    def pop(self):
        return self.records.popleft()
