import queue
from threading import Thread
from traceback import format_exc
from collections import namedtuple
import logging.handlers
import sys
import os
from stat import ST_DEV, ST_INO

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalCountersUpdater


class DeferCaller:
    def __init__(self, queue_size, block):
        self._queue = queue.Queue(queue_size)
        self._thread = None
        self._block = block
        self.callbacks_count = 0
        self.errors_count = 0
        self.dropped_count = 0

    def __call__(self, f):
        self._init()
        try:
            self._queue.put(f, block=self._block)
        except queue.Full:
            self.dropped_count += 1

    def join(self):
        self._queue.join()

    def _init(self):
        if not (self._thread and self._thread.is_alive()):
            self._thread = Thread(target=self._loop, name=self.__class__.__name__)
            self._thread.daemon = True
            self._thread.start()

    def _loop(self):
        while True:
            f = self._queue.get()
            try:
                f()
                self.callbacks_count += 1
            except Exception as e:
                self.errors_count += 1
                try:
                    sys.stderr.write('[{0}] defer caller error {1}\n{2}'.format(os.getpid(), e, format_exc()))
                    sys.stderr.flush()
                except:  # noqa
                    pass
            self._queue.task_done()


class FileWriter:
    def __init__(self, file_name):
        self._file_name = file_name
        self._file = self._open()
        self.dev, self.ino = -1, -1
        self._stat_file()

    def write(self, m):
        self._reopen_if_needed()
        if self._file is None:
            self._file = self._open()
        fs = '%s\n'
        self._file.write((fs % m).encode("utf-8"))
        self._file.flush()

    def _open(self):
        return open(self._file_name, 'ab', 0)

# copypasted from WatchedFileHandler
    def _stat_file(self):
        if self._file:
            sres = os.fstat(self._file.fileno())
            self.dev, self.ino = sres[ST_DEV], sres[ST_INO]

    def _reopen_if_needed(self):
        """
        Reopen log file if needed.

        Checks if the underlying file has changed, and if it
        has, close the old stream and reopen the file to get the
        current stream.
        """
        # Reduce the chance of race conditions by stat'ing by path only
        # once and then fstat'ing our new fd if we opened a new log stream.
        # See issue #14632: Thanks to John Mulligan for the problem report
        # and patch.
        try:
            # stat the file by path, checking for existence
            sres = os.stat(self._file_name)
        except FileNotFoundError:
            sres = None
        # compare file system stat with that of our stream file handle
        if not sres or sres[ST_DEV] != self.dev or sres[ST_INO] != self.ino:
            if self._file is not None:
                # we have an open file handle, clean it up
                self._file.flush()
                self._file.close()
                self._file = None  # See Issue #21742: _open () might fail.
                # open a new file handle and get new stat info from that fd
                self._file = self._open()
                self._stat_file()


class AsyncWatchedFileHandler(logging.Handler):
    def __init__(self, defer_caller, file_writer, *args, **kwargs):
        super(AsyncWatchedFileHandler, self).__init__(*args, **kwargs)
        self._defer_caller = defer_caller
        self._file_writer = file_writer

    def acquire(self):
        pass

    def release(self):
        pass

    def emit(self, record):
        self._defer_caller(lambda: self._file_writer.write(self.format(record)))


AsyncFileLoggerStats = namedtuple('AsyncFileLoggerStats',
                                  ['messages_count', 'errors_count', 'dropped_messages_count'])


class AsyncFileLogger:
    def __init__(self, log_file):
        self._defer_caller = DeferCaller(log_file.get('queue_size', 10000),
                                         log_file.get('block_on_full_queue', False))
        self._file_writer = FileWriter(log_file['file_name'])
        GlobalCountersUpdater.register(self._update_counters)

    def create_handler(self):
        return AsyncWatchedFileHandler(self._defer_caller, self._file_writer)

    def get_stats(self):
        return AsyncFileLoggerStats(self._defer_caller.callbacks_count,
                                    self._defer_caller.errors_count,
                                    self._defer_caller.dropped_count)

    def join(self):
        self._defer_caller.join()

    def _update_counters(self):
        stats = self.get_stats()
        counters = GlobalCounter
        counters.ASYNC_FILE_LOGGER_MESSAGES_COUNT_SUMM.set(stats.messages_count)
        counters.ASYNC_FILE_LOGGER_ERRORS_COUNT_SUMM.set(stats.errors_count)
        counters.ASYNC_FILE_LOGGER_DROPPED_MESSAGES_COUNT_SUMM.set(stats.dropped_messages_count)
