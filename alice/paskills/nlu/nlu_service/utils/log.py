# coding: utf-8

import json
import logging
import logging.config
import sys
import traceback
from cStringIO import StringIO


class JsonLogFormatter(logging.Formatter):

    def format(self, record):
        entry = {
            'msg': record.getMessage(),
            'level': record.levelname.lower(),
            '@fields': {
                'line': record.lineno,
                'path': record.pathname,
                'function': record.funcName,
            }
        }
        if any(sys.exc_info()):
            buffer = StringIO()
            traceback.print_exc(file=buffer)
            entry['stackTrace'] = buffer.getvalue()
            buffer.close()
        return json.dumps(entry)


def setup_logging(line):
    if line:
        logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', datefmt='%Y-%m-%d %H:%M:%S', level=logging.INFO)
    else:
        config = {
            'version': 1,
            'disable_existing_loggers': False,
            'formatters': {
                'json': {
                    '()': 'nlu_service.utils.log.JsonLogFormatter',
                },
            },
            'handlers': {
                'default': {
                    'level': 'INFO',
                    'formatter': 'json',
                    'class': 'logging.StreamHandler',
                },
            },
            'loggers': {
                '': {
                    'handlers': ['default'],
                    'level': 'INFO',
                    'propagate': True
                },
            }
        }

        logging.config.dictConfig(config)
