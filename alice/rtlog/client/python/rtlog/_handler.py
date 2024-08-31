import logging
from six.moves import collections_abc
import sys
from .thread_local import _get_request_logger
from traceback import format_exc


class RTLogHandler(logging.Handler):
    _error_levels = [logging.CRITICAL, logging.FATAL, logging.ERROR]
    _record_fields = ['name', 'threadName', 'lineno', 'funcName', 'exc_info']

    def emit(self, record):
        if getattr(record, 'rtlog_ignore', False):
            return
        request_logger = getattr(record, 'rt_log', None)
        if not request_logger:
            request_logger = _get_request_logger()
        if not request_logger:
            return
        try:
            severity = 'error' if record.levelno in RTLogHandler._error_levels else 'info'
            fields = {}
            for f in RTLogHandler._record_fields:
                v = getattr(record, f)
                if v:
                    fields[f] = v
            if isinstance(record.args, collections_abc.Mapping):
                message = record.msg
                fields.update(record.args)
            else:
                message = getattr(record, 'message', None)
                if message is None:
                    message = record.getMessage()
            request_logger.log_event(record.created, severity, message, **fields)
        except Exception as e:
            sys.stderr.write('RTLogHandler error {0}\n{1}'.format(e, format_exc()))
