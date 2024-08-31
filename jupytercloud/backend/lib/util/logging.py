import abc
import functools
import inspect
import json
import logging
import logging.handlers
import os
from contextlib import contextmanager
from contextvars import ContextVar
from copy import copy
from pathlib import Path

from jupyterhub.log import CoroutineLogFormatter, _scrub_uri
from tornado.ioloop import IOLoop
from tornado.log import access_log
from tornado.web import HTTPError
from traitlets import MetaHasTraits
from traitlets.config.application import Application

from jupytercloud.backend.lib.db.util import setup_execute_logging
from library.python.deploy_formatter import DeployFormatter
from logbroker.unified_agent.client.python import UnifiedAgentYdHandler


DEFAULT_STDOUT_LOG_FORMAT = (
    '%(color)s[%(levelname)1.1s %(asctime)s.%(msecs).03d %(pathname)s:%(lineno)d]%(end_color)s%(context)s %(message)s'
)
DEFAULT_DATE_FORMAT = '%Y-%m-%d %H:%M:%S'

LOGGING_CONTEXT = ContextVar('jupytercloud_logging_context', default=None)


class StdoutFormatter(CoroutineLogFormatter):
    def format(self, record):
        record_format = copy(record)
        record_format.pathname = record_format.pathname.split('site-packages')[-1]

        context = get_logging_context()
        if context:
            record_format.context = ' ' + json.dumps(get_logging_context())
        else:
            record_format.context = ''

        return super().format(record_format)


def create_handler(filename, level=logging.NOTSET):
    log_dir = filename.parent
    if not log_dir.exists():
        log_dir.mkdir(parents=True)

    handler = logging.handlers.WatchedFileHandler(filename)
    handler.setLevel(level)

    return handler


def generate_file_handlers(log_dir, formatter):
    handlers = []
    for log_level, level_name in (
        (logging.ERROR, 'error'),
        (logging.WARNING, 'warning'),
        (logging.INFO, 'info'),
        (logging.DEBUG, 'debug'),
    ):
        filename = (log_dir / level_name).with_suffix('.log')
        handler = create_handler(filename, log_level)
        handlers.append(handler)
        handler.setFormatter(formatter)

    return handlers


def setup_asyncio_logging(
    path,
    formatter,
    log_level=logging.INFO,
):
    tornado_loop = IOLoop.current()
    loop = tornado_loop.asyncio_loop
    loop.set_debug(True)

    logger = logging.getLogger('asyncio')
    logger.setLevel(log_level)

    handler = create_handler(path, log_level)
    handler.setFormatter(formatter)

    logger.addHandler(handler)


def setup_db_logging(path, formatter, log_level=logging.WARNING):
    logger = logging.getLogger('sqlalchemy')
    logger.setLevel(log_level)
    logger.propagate = False

    handler = create_handler(path, log_level)
    handler.setFormatter(formatter)

    logger.addHandler(handler)

    engine_logger = logging.getLogger('sqlalchemy.engine')
    engine_logger.setLevel(max(log_level, logging.INFO))

    if log_level == logging.DEBUG:
        setup_execute_logging()


def setup_additional_loggers(logger, setup_tornado_logging, additional_loggers, log_level):
    additional_loggers = tuple(additional_loggers)
    if setup_tornado_logging:
        # disable curl debug, which is TOO MUCH
        logging.getLogger('tornado.curl_httpclient').setLevel(max(log_level, logging.INFO))

        from tornado.log import access_log, app_log, gen_log

        for log in (app_log, access_log, gen_log):
            # ensure all log statements identify the application they come from
            log.name = logger.name

        additional_loggers = additional_loggers + ('tornado',)

    for log_name in additional_loggers:
        additional_logger = logging.getLogger(log_name)
        additional_logger.propagate = True
        additional_logger.parent = logger


def generate_stdout_handler(log_format, date_format, log_level):
    formatter = StdoutFormatter(
        fmt=log_format,
        datefmt=date_format,
    )

    handler = logging.StreamHandler()
    handler.setFormatter(formatter)
    handler.setLevel(log_level)

    return handler


def set_context(record):
    # In Ya.Deploy it will be context.context, actually, as its logger already has a 'context' field
    record.context = get_logging_context()
    return True


def setup_jupytercloud_logging(
    application_class_name,
    log_dir,
    setup_tornado_logging=True,
    additional_loggers=('sentry_sdk',),
    log_level=logging.DEBUG,
    stdout_log_level=None,
    asyncio_log_level=None,
    db_log_level=None,
):
    app = Application.instance()
    class_name = app.__class__.__name__

    if application_class_name != class_name:
        return

    stdout_log_level = stdout_log_level or log_level
    asyncio_log_level = asyncio_log_level or max(log_level, logging.INFO)
    db_log_level = db_log_level or max(log_level, logging.WARNING)

    log_dir = Path(log_dir)

    logger = logging.getLogger(class_name)
    logger.setLevel(logging.DEBUG)
    logger.propagate = False
    logger.addFilter(set_context)

    setup_additional_loggers(
        logger,
        setup_tornado_logging,
        additional_loggers,
        log_level,
    )

    stdout_handler = generate_stdout_handler(
        log_format=DEFAULT_STDOUT_LOG_FORMAT,
        date_format=DEFAULT_DATE_FORMAT,
        log_level=stdout_log_level,
    )
    logger.addHandler(stdout_handler)

    file_formatter = StdoutFormatter(
        fmt=DEFAULT_STDOUT_LOG_FORMAT,
        datefmt=DEFAULT_DATE_FORMAT,
    )

    setup_asyncio_logging(
        path=log_dir / 'asyncio.log',
        formatter=file_formatter,
        log_level=asyncio_log_level,
    )
    setup_db_logging(
        path=log_dir / 'db.log',
        formatter=file_formatter,
        log_level=db_log_level,
    )

    for handler in generate_file_handlers(log_dir=log_dir, formatter=file_formatter):
        logger.addHandler(handler)

    if 'DEPLOY_POD_ID' in os.environ:
        unified_agent_handler = UnifiedAgentYdHandler('stdout', 'localhost:12500')
        unified_agent_handler.setFormatter(DeployFormatter())
        logger.addHandler(unified_agent_handler)

    app.log = logger


def update_logging_context(**kwargs):
    context = LOGGING_CONTEXT.get()
    # potentially dangerous place, may need a deepcopy
    # because when context splits, it inherits a dict
    # object from previous context
    context = context.copy() if context else {}
    context.update(kwargs)
    LOGGING_CONTEXT.set(context)


def get_logging_context():
    return LOGGING_CONTEXT.get() or {}


@contextmanager
def with_logging_context(**kwargs):
    old_context = get_logging_context()
    update_logging_context(**kwargs)

    try:
        yield
    except BaseException as e:
        e.context = get_logging_context()
        raise e from e
    finally:
        LOGGING_CONTEXT.set(old_context)


class LoggingContextMeta(MetaHasTraits, abc.ABCMeta):
    def __init__(cls, name, bases, classdict):
        super().__init__(name, bases, classdict)

        assert hasattr(
            cls, 'log_context',
        ), f'class {cls} must have .log_context attribute/property to use LoggingContextMeta'

        for attr, value in classdict.items():
            # "if" order matters!
            if inspect.isasyncgenfunction(value):
                decorator = cls.async_generator_log_context
            elif inspect.iscoroutinefunction(value):
                decorator = cls.async_method_log_context
            elif inspect.isfunction(value):
                decorator = cls.method_log_context
            else:
                continue

            setattr(cls, attr, decorator(value))

    @staticmethod
    def get_method_context(instance, method):
        context = instance.log_context or {}
        context.update({
            'class': instance.__class__.__name__,
            'method': method.__name__,
        })
        return context

    def method_log_context(cls, func):
        @functools.wraps(func)
        def _method(self, *args, **kwargs):
            with with_logging_context(
                **cls.get_method_context(self, func),
            ):
                return func(self, *args, **kwargs)

        return _method

    def async_method_log_context(cls, func):
        @functools.wraps(func)
        async def _method(self, *args, **kwargs):
            with with_logging_context(
                **cls.get_method_context(self, func),
            ):
                return await func(self, *args, **kwargs)

        return _method

    def async_generator_log_context(cls, func):
        @functools.wraps(func)
        async def _method(self, *args, **kwargs):
            with with_logging_context(
                **cls.get_method_context(self, func),
            ):
                async for _ in func(self, *args, **kwargs):
                    yield _

        return _method


class LoggingContextMixin(metaclass=LoggingContextMeta):
    @abc.abstractproperty
    def log_context(self):
        raise NotImplementedError()


# Function adapted from jupyterhub.log.log_request (BSD) to use in our logging and sentry
# integration.
# It is supposed to be used at c.JupyterHub.tornado_settings = {'log_request': log_request}
# at config file.
def log_request(handler):
    status = handler.get_status()
    request = handler.request
    if status == 304 or status < 300:
        # all GETs are very noisy, move it to DEBUG
        log_method = access_log.debug
    elif status < 400:
        log_method = access_log.info
    elif status < 500:
        log_method = access_log.warning
    else:
        log_method = access_log.error

    uri = _scrub_uri(request.uri)

    # XXX: silensing message for sentry
    # "Failing suspected API request to not-running server: /hub/user/<user>/metrics"
    if status == 503 and uri.endswith('/metrics'):
        log_method = access_log.warning

    request_time = 1000.0 * handler.request.request_time()

    try:
        user = handler.current_user
    except (HTTPError, RuntimeError):
        username = ''
    else:
        if user is None:
            username = ''
        elif isinstance(user, str):
            username = user
        elif isinstance(user, dict):
            username = user['name']
        else:
            username = user.name

    ns = dict(
        status=status,
        method=request.method,
        ip=request.remote_ip,
        uri=uri,
        request_time=request_time,
        user=username,
        location='',
    )
    msg = '%(status)s %(method)s %(uri)s%(location)s (%(user)s@%(ip)s) %(request_time).2fms'
    if status in {301, 302}:
        location = handler._headers.get('Location')
        if location:
            ns['location'] = f' -> {_scrub_uri(location)}'
    log_method(msg, ns)
