# coding: utf-8
from __future__ import unicode_literals, absolute_import

import errno
import socket


def unwind_exception(exc):
    res = []

    def _inner(exc):
        res.append(exc)
        for item in exc.args:
            if isinstance(item, Exception):
                _inner(item)

    _inner(exc)
    return res


def is_reset_by_peer_exc(exception):
    exceptions = unwind_exception(exception)

    for exc in reversed(exceptions):
        if isinstance(exc, socket.error) and exc.args[0] == errno.ECONNRESET:
            return True

    return False
