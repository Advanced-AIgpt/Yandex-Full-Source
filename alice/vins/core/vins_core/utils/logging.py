# coding: utf-8
from __future__ import unicode_literals, absolute_import

import logging
import logging.handlers

from time import time
from contextlib import contextmanager
from Queue import Queue
from threading import Thread

from ylog.context import get_log_context, log_context


logger = logging.getLogger(__name__)


REQUEST_FIELDS_BLACKLIST = [
    'request.additional_options.oauth_token',
    'request.additional_options.bass_options.cookies',
]


@contextmanager
def log_call(name):
    start_time = time()
    data = {'name': name}
    try:
        yield
    except BaseException as e:
        data['error_type'] = type(e).__name__
        data['error'] = unicode(e)
        raise
    finally:
        elapsed_time = time() - start_time
        data['elapsed_time'] = elapsed_time
        status = 'failed' if 'error' in data else 'succeeded'
        logger.info(
            'Operation "%s" has %s after %.0fms',
            name,
            status,
            elapsed_time * 1000,
            extra={'data': data}
        )


class _LazyLogging(object):
    """ Lazy logging function decorator.
    Useful for logging heavy functions.

    """
    def __init__(self, f, *args, **kwargs):
        self._f = f
        self._args = args
        self._kwargs = kwargs
        self._str = None

    def __call__(self, *args, **kwargs):
        return self.__class__(self._f, *args, **kwargs)

    def __unicode__(self):
        if self._str is None:
            self._str = unicode(self._f(*self._args, **self._kwargs))
        return self._str

    def __str__(self):
        return unicode(self).encode('utf-8')


lazy_logging = _LazyLogging


class AsyncHandlerMixin(object):
    def __init__(self, queue_size=-1, *args, **kwargs):
        super(AsyncHandlerMixin, self).__init__(*args, **kwargs)
        self._queue = Queue(queue_size)
        self._thread = None

    def _init(self):
        if not (self._thread and self._thread.is_alive()):
            self._thread = Thread(target=self._loop, name=self.__class__.__name__)
            self._thread.daemon = True
            self._thread.start()

    def emit(self, record):
        self._init()
        self._queue.put((record, get_log_context()))

    def _loop(self):
        while True:
            record, context = self._queue.get()
            with log_context(**context):
                try:
                    super(AsyncHandlerMixin, self).emit(record)
                except Exception:
                    pass


class NonBufferedWatchedFileHandler(logging.handlers.WatchedFileHandler):
    def _open(self):
        return open(self.baseFilename, self.mode, 0)


class AsyncStreamHandler(AsyncHandlerMixin, logging.StreamHandler):
    pass


class AsyncSysLogHandler(AsyncHandlerMixin, logging.handlers.SysLogHandler):
    pass


class AsyncWatchedFileHandler(AsyncHandlerMixin, NonBufferedWatchedFileHandler):
    pass
