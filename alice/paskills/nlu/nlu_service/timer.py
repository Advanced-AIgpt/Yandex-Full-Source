# coding: utf-8

import time


class Timer(object):
    """A context manager that can track execution time"""

    def __init__(self):
        self._start = None
        self._end = None

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.stop()

    def start(self):
        self._start = time.time()

    def stop(self):
        self._end = time.time()
        return self.time

    @property
    def time(self):
        return self._end - self._start
