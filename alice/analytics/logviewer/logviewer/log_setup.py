# -*- coding: utf-8 -*-
from os.path import join


def make_log_config(log_dir):
    return {
        'version': 1,
        'disable_existing_loggers': True,

        'formatters': {
            'verbose': {
                'format': '%(levelname)s %(asctime)s %(module)s.%(funcName)s: %(message)s'
            },
            'simple': {
                'format': '%(levelname)s %(message)s'
            },
        },

        'filters': {
            'require_debug_false': {
                '()': 'django.utils.log.RequireDebugFalse',
            },
            'require_debug_true': {
                '()': 'django.utils.log.RequireDebugTrue',
            },
        },

        'handlers': {
            'null': {
                'level': 'DEBUG',
                'class': 'logging.NullHandler',
            },
            'console': {
                'level': 'DEBUG',
                'class': 'logging.StreamHandler',
                'formatter': 'verbose',
                'filters': ['require_debug_true']
            },
            'file': {
                'level': 'WARNING',
                'class': 'logging.handlers.RotatingFileHandler',
                'filename': join(log_dir, 'logviewer.log'),
                'filters': ['require_debug_false'],
                'formatter': 'verbose',
                'maxBytes': 10*1024,
                'backupCount': 5
            },
        },

        'loggers': {
            '': {
                'handlers': ['console'],
                'propagate': True,
                'level': 'DEBUG',
            },
            'django': {
                'handlers': ['file'],
                'propagate': True,
                'level': 'INFO',
            },
        }
    }

