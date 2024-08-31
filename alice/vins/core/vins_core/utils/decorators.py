# coding: utf-8
from __future__ import unicode_literals, absolute_import

import cProfile
import logging
import pstats

from time import time
from StringIO import StringIO
from functools import wraps

from vins_core.utils.config import get_setting

logger = logging.getLogger(__name__)


def profile_time(name=None):
    def wrapper(f):

        @wraps(f)
        def wrapped(*args, **kwargs):
            start_time = time()
            result = f(*args, **kwargs)
            end_time = time()

            dt = end_time - start_time
            if dt > 0.1:
                logger.warning(
                    'FUNC "%s" TOO SLOW . dt=%0.3f (s.)',
                    name, dt
                )
            return result

        return wrapped
    return wrapper


def cprofile_time(name=None, limit=1):
    def wrapper(f):

        @wraps(f)
        def wrapped(*args, **kwargs):
            pr = cProfile.Profile()
            pr.enable()
            start_time = time()
            result = f(*args, **kwargs)
            end_time = time()
            pr.disable()
            out = StringIO()
            ps = pstats.Stats(pr, stream=out).sort_stats('cumulative')

            dt = end_time - start_time
            if dt > limit:
                ps.print_stats()
                logger.warning(
                    'FUNC "%s" TOO SLOW . dt=%0.3f (s.)',
                    name, dt
                )
                logger.warning(out.getvalue())
            return result

        return wrapped
    return wrapper


def time_info(name=None):
    def wrapper(f):

        @wraps(f)
        def wrapped(*args, **kwargs):
            start_time = time()
            result = f(*args, **kwargs)
            end_time = time()

            dt = end_time - start_time
            logger.debug('TIME_DEBUG\t"%s"\t%0.3f', name, dt)
            return result

        if get_setting('TIME_DEBUG', False):
            return wrapped
        else:
            return f
    return wrapper


def retry_on_exception(exception, pred=None, retries=1):
    """
    Retry decorated function `retries` times if function raise an
    `exception` and `pred` on this exception is true.

    """
    assert retries >= 0

    def decorator(f):
        @wraps(f)
        def inner(*args, **kwargs):
            attempt = 0
            while True:
                try:
                    return f(*args, **kwargs)
                except exception as exc:
                    if pred is None or pred(exc):
                        if attempt >= retries:
                            raise
                        else:
                            logger.warning('Got a retriable exception %s.', exc, exc_info=True)
                            attempt += 1
                    else:
                        raise
        return inner
    return decorator
