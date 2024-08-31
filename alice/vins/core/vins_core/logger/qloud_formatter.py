import logging
import json
import time
import socket
from ylog.format import QloudJsonFormatter as QloudJsonFormatterBase
from ylog.context import get_log_context
from vins_core.utils.config import get_setting


hostname = socket.getfqdn()


class QloudJsonFormatter(QloudJsonFormatterBase):
    LOG_RECORD_USEFUL_FIELDS = ('funcName', 'lineno', 'name', 'data')


class RawQloudJsonFormatter(logging.Formatter):
    LOG_RECORD_USEFUL_FIELDS = ('funcName', 'lineno', 'name', 'data')

    def format(self, record):
        record.message = record.getMessage()

        tz = time.altzone if time.daylight and time.localtime().tm_isdst > 0 else time.timezone
        tz_formatted = '{}{:0>2}{:0>2}'.format('-' if tz > 0 else '+', abs(tz) // 3600, abs(tz // 60) % 60)

        log_data = {
            'level': record.levelno,
            'levelStr': record.levelname,
            'loggerName': record.name,
            '@version': 0,
            'threadName': record.threadName if record.threadName else '',
            '@timestamp': time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(record.created)) + tz_formatted,
            'qloud_project': get_setting('QLOUD_PROJECT'),
            'qloud_application': get_setting('QLOUD_APPLICATION'),
            'qloud_environment': get_setting('QLOUD_ENVIRONMENT'),
            'qloud_component': get_setting('QLOUD_COMPONENT'),
            'qloud_instance': '-',
            'message': record.message,
            'host': hostname
        }

        if record.exc_info:
            exc = logging.Formatter.formatException(self, record.exc_info)
            log_data['stackTrace'] = exc

        fields = {}

        standard_fields = self._get_standard_fields(record)
        if standard_fields:
            fields['std'] = standard_fields

        log_context_fields = get_log_context()
        if log_context_fields:
            fields['context'] = log_context_fields

        if fields:
            log_data['@fields'] = fields

        return json.dumps(log_data)

    def _get_standard_fields(self, record):
        return {
            field: getattr(record, field)
            for field in self.LOG_RECORD_USEFUL_FIELDS
            if hasattr(record, field)
        }
