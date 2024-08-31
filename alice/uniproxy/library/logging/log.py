import logging
import logging.handlers
import tornado.gen

import sys
import codecs

if sys.version_info[0] < 3:
    sys.stdout = codecs.getwriter('utf8')(sys.stdout)
    sys.stderr = codecs.getwriter('utf8')(sys.stderr)
logging._defaultFormatter = logging.Formatter(u"%(message)s")


class SingleLineFormatter(logging.Formatter):
    datefmt = '%Y-%m-%d %H:%M:%S'

    def format(self, record):
        s = "%s %s %s: %s" % (
            record.name,
            record.levelname,
            self.formatTime(record, self.datefmt),
            record.getMessage()
        )
        if record.exc_info:
            record.exc_text = self.formatException(record.exc_info).replace("\n", "")
        if record.exc_text:
            s = s + record.exc_text
        return s


class MainLogger(logging.getLoggerClass()):
    _sentry_client = None

    def debug(self, *message, rt_log=None):
        try:
            super().debug("%s " * len(message), *message, extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            print(*message)

    def info(self, *message, rt_log=None):
        try:
            super().info("%s " * len(message), *message, extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            print(*message)

    def warning(self, *message, rt_log=None):
        try:
            super().warning("%s " * len(message), *message, extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            print(*message)

    def error(self, *message, rt_log=None):
        try:
            super().error("%s " * len(message), *message, extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            print(*message)

    @tornado.gen.engine
    def exception(self, exc, rt_log=None):
        if exc is not None:
            try:
                super().exception(exc, extra={'rt_log': rt_log} if rt_log else None)
            except Exception:
                print(exc)
            try:
                if MainLogger._sentry_client:
                    yield tornado.gen.Task(
                        MainLogger._sentry_client.captureException, exc_info=True
                    )
            except Exception as exc:
                print("Exception on sentry captureException:", str(exc))
        else:
            self.error("EXC called with None", rt_log=rt_log)


class Logger:
    _log = None
    _access_log = None
    _access_log_ex = None
    _session_log = None
    _msg_log = None
    _default_streamHandler = None
    _loggers = {}
    _app_name = None
    _additional_log_params = {
        'gcmon_log_level': logging.DEBUG,
        'memcached_log_level': logging.DEBUG,
        'mssngr_log_level': logging.DEBUG,
        'qtvmcli_log_level': logging.DEBUG,
        'subway_log_level': logging.INFO,
        'delivery_log_level': logging.INFO,
    }

    @classmethod
    def init(cls, app_name, is_debug, sentry_client=None, **kwargs):
        MainLogger._sentry_client = sentry_client

        _def_log_lvl = logging.INFO if app_name == 'notificator' else logging.ERROR
        default_log_lvl = _def_log_lvl if not is_debug else logging.DEBUG

        logging.setLoggerClass(MainLogger)

        cls._app_name = app_name
        cls._additional_log_params.update(kwargs)

        cls._log = logging.getLogger(app_name)
        cls._log.propagate = False
        cls._log.setLevel(default_log_lvl)

        streamHandler = cls._create_handler()
        streamFormatter = SingleLineFormatter()
        streamHandler.setFormatter(streamFormatter)

        cls._log.addHandler(streamHandler)

        cls._access_log = cls.get("tornado.access")
        cls._access_log.setLevel(logging.WARNING if app_name == 'subway' else default_log_lvl)

        cls._access_log_ex = logging.getLogger()
        cls._access_log_ex.setLevel(logging.INFO)
        cls._access_log_ex.addHandler(cls._create_handler())

        cls._session_log = logging.getLogger(app_name + '.session')
        stdoutHandler = cls._create_handler()
        stdoutFormatter = logging.Formatter(u"SESSIONLOG: %(message)s")
        stdoutHandler.setFormatter(stdoutFormatter)
        cls._session_log.addHandler(stdoutHandler)
        cls._session_log.setLevel(logging.INFO)
        cls._session_log.propagate = False

        cls._msg_log = logging.getLogger(app_name + '.message')
        outHandler = cls._create_handler()
        outFormatter = logging.Formatter(u'MSGLOG (%(asctime)s) %(message)s')
        outHandler.setFormatter(outFormatter)
        cls._msg_log.addHandler(outHandler)
        cls._msg_log.setLevel(default_log_lvl)
        cls._msg_log.propagate = False

        application_log = logging.getLogger("tornado.application")
        application_log.setLevel(default_log_lvl)
        application_log.addHandler(streamHandler)

        general_log = logging.getLogger("tornado.general")
        general_log.setLevel(default_log_lvl)
        general_log.addHandler(streamHandler)

    @classmethod
    def register_rtlog_handler(cls, handler):
        cls._log.addHandler(handler)
        cls._session_log.addHandler(handler)
        cls._access_log.addHandler(handler)
        cls._access_log_ex.addHandler(handler)

    @classmethod
    def get(cls, name=None):
        if not name:
            return cls._log or logging.getLogger()

        top_lvl_name = name.split('.')[0]
        if top_lvl_name == '':
            top_lvl_name = cls._app_name or ''
            name = top_lvl_name + name

        if top_lvl_name in cls._loggers:
            return logging.getLogger(name)

        log_level_key = '%s_log_level' % top_lvl_name
        log_level = cls._additional_log_params.get(log_level_key, logging.INFO)
        logger = logging.getLogger(name)
        logger.setLevel(log_level)
        if cls._default_streamHandler is not None:
            logger.addHandler(cls._default_streamHandler)
        logger.propagate = False
        cls._loggers[top_lvl_name] = logger
        return logger

    @classmethod
    def _create_handler(cls):
        async_file_logger = cls._additional_log_params.get('async_file_logger', None)
        return async_file_logger.create_handler() if async_file_logger else logging.StreamHandler(sys.stdout)

    @classmethod
    def session(cls, message, rt_log=None):
        try:
            cls._session_log.info(message, rt_log=rt_log)
        except Exception as exc:
            cls._log.exception(exc)

    @classmethod
    def access(cls, url, code, client_ip, request_time, rt_log=None):
        try:
            cls._access_log.info("%s POST /%s (%s) %dms" % (code, url, client_ip, request_time),
                                 extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            pass

    @classmethod
    def access_ex(cls, message, rt_log=None):
        try:
            cls._access_log_ex.info(message, extra={'rt_log': rt_log} if rt_log else None)
        except Exception:
            pass

    @classmethod
    def msglog(
        cls,
        stage,
        payload_id,
        uuid_from='N/A',
        uuid_to='N/A',
        guid_from='N/A',
        guid_to='N/A',
        chat_to='N/A',
        location='N/A'
    ):
        try:
            cls._msg_log.info(
                '%-40s %-20s GUID:%s => (GUID:%s CHAT:%s) UUID:%s => UUID:%s LOC:%s',
                payload_id, stage, guid_from, guid_to, chat_to, uuid_from, uuid_to, location
            )
        except Exception:
            pass
