# coding:utf-8
import logging
from queue import Queue
from threading import Thread

import pytz
import ujson

from datetime import datetime


def get_fields(record):
    fields = {
        'level': record.levelname.lower(),
        'time': datetime.fromtimestamp(record.created, tz=pytz.UTC).strftime('%Y-%m-%dT%H:%M:%S.%f%z'),
        'caller': '{module}/{filename}:{lineno}'.format(
            module=record.module,
            filename=record.filename,
            lineno=record.lineno
        ),
        'message': record.getMessage().encode('utf-8'),
    }
    if hasattr(record, 'uuid'):
        fields['uuid'] = record.uuid
    if hasattr(record, 'session_id'):
        fields['session_id'] = record.session_id
    if hasattr(record, 'skill_id'):
        fields['skill_id'] = record.skill_id
    return fields


class AsyncHandlerMixin:
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
        self._queue.put(record)

    def _loop(self):
        while True:
            record = self._queue.get()
            try:
                super(AsyncHandlerMixin, self).emit(record)
            except:
                pass


class AsyncStreamHandler(AsyncHandlerMixin, logging.StreamHandler):
    pass


class ConsoleSkillFormatter(logging.Formatter):
    def format(self, record):
        super(ConsoleSkillFormatter, self).format(record)
        return ' '.join('{}={}'.format(key, value) for key, value in get_fields(record).items())


class JsonSkillFormatter(logging.Formatter):
    def format(self, record):
        super(JsonSkillFormatter, self).format(record)
        return ujson.dumps(get_fields(record), ensure_ascii=False, escape_forward_slashes=False)


def create_config(level, log_json):
    formatter_class = 'gamma_sdk.log.JsonSkillFormatter' if log_json else 'gamma_sdk.log.ConsoleSkillFormatter'
    config = {
        'version': 1,
        'disable_existing_loggers': True,
        'formatters': {
            'skill_formatter': {
                '()': formatter_class,
            }
        },
        'handlers': {
            'skill_handler': {
                'class': 'gamma_sdk.log.AsyncStreamHandler',
                'formatter': 'skill_formatter',
            },
        },
        'loggers': {
            'skill_logger': {
                'handlers': ['skill_handler'],
                'level': level,
                'propagate': True
            }
        }
    }
    return config


def get_log_adapter(logger, **extra):
    return logging.LoggerAdapter(logger, extra)
