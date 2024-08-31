import logging.config


def set_default_logging(level='INFO'):
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'formatter': 'standard',
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': level,
                'propagate': True,
            },
        },
    })
