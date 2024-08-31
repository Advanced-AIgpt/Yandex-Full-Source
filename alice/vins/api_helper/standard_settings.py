from vins_core.dm.request import get_experiments
from vins_core.utils.config import get_setting, get_bool_setting
from rtlog import is_active as rtlog_is_active


def extract_uuid(request):
    return {
        'uuid': request.GET.get('uuid') or request.META.get('HTTP_X_UUID')
    }


def get_root_logger_handlers():
    res = ['console']

    if not get_bool_setting('DISABLE_SENTRY'):
        res.append('sentry')

    if rtlog_is_active():
        res.append('rtlog')

    return res


LOG_REQUEST_ID_HEADER = "HTTP_X_REQUEST_ID"
REQUEST_ID_RESPONSE_HEADER = "X-Response-ID"

MONGODB_URL = get_setting('MONGODB_URL', default='mongodb://localhost/')
MONGODB_NAME = get_setting('MONGODB_NAME', 'vins_dm')

SENTRY_DSN = get_setting('SENTRY_DSN', default='')


def get_console_formatter():
    parser = get_setting('QLOUD_LOGGER_STDOUT_PARSER', '', prefix='')
    if parser == 'json':
        return 'qloud_json'
    if parser == 'json_raw':
        return 'qloud_json_raw'
    return 'standard'


def get_console_handler():
    result = {
        'queue_size': 10000,
        'level': get_setting('DJANGO_LOGLEVEL', 'DEBUG'),
        'formatter': get_console_formatter()
    }
    log_file = get_setting('LOG_FILE', '')
    if log_file:
        result['class'] = 'vins_core.utils.logging.AsyncWatchedFileHandler'
        result['filename'] = log_file
        result['mode'] = 'a'
    else:
        result['class'] = 'vins_core.utils.logging.AsyncStreamHandler'
        result['stream'] = 'ext://sys.stdout'
    return result


LOGGING = {
    'version': 1,
    'disable_existing_loggers': False,
    'formatters': {
        'standard': {
            'format': '%(asctime)s|%(levelname)-8s|%(module)s: %(message)s',
            'datefmt': '%Y-%m-%d_%H:%M:%S'
        },
        'colored': {
            '()': 'vins_core.logger.color_formatter.ColorFormatter',
            'format': '$COLOR[%(asctime)s] [%(module)s:%(lineno)d] [%(levelname)s] %(message)s',
            'datefmt': '%Y-%m-%d_%H:%M:%S'
        },
        'qloud_json': {
            '()': 'vins_core.logger.QloudJsonFormatter',
        },
        'qloud_json_raw': {
            '()': 'vins_core.logger.RawQloudJsonFormatter',
        },
        'message_only': {
            'format': '%(message)s',
        },
    },
    'handlers': {
        'console': get_console_handler(),
        'sentry': {
            'level': 'ERROR',
            'class': 'raven.handlers.logging.SentryHandler',
            'dsn': SENTRY_DSN,
        },
        'rtlog': {
            'class': 'rtlog.RTLogHandler'
        },
        'features_file': {
            'class': 'vins_core.utils.logging.AsyncWatchedFileHandler',
            'filename': get_setting('FEATURES_LOG_FILE', default='./vins-features.log'),
            'mode': 'a',
            'formatter': get_console_formatter()
        }
    },
    'loggers': {
        'dialog_history': {
            'handlers': ['console'],
            'propagate': False,
        },
        # disable debug transition model by default
        # because of broken logs in testing
        'transition_model': {
            'handlers': ['console'],
            'level': get_setting('TRANSITION_MODEL_LOGLEVEL', 'CRITICAL'),
            'propagate': False,
        },
        'features': {
            'handlers': ['features_file'],
            'propagate': False,
        },
        '': {
            'handlers': get_root_logger_handlers(),
            'level': 'DEBUG',
            'propagate': True,
        }
    },
}
if get_bool_setting('PUSH_LOGS_TO_YT', False):
    LOGGING['handlers']['syslog_short'] = {
        'class': 'vins_core.utils.logging.AsyncSysLogHandler',
        'queue_size': 10000,
        'address': '/dev/log',
        'formatter': 'message_only',
        'level': 'DEBUG',
    }
    LOGGING['loggers']['dialog_history']['handlers'] = ['syslog_short']


EXPERIMENTS = get_experiments()
