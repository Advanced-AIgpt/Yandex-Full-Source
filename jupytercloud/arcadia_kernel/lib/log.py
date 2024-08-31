# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import logging

from traitlets import Unicode, List, Instance, Any, default
from traitlets.config import LoggingConfigurable


class FileLogMixin(LoggingConfigurable):
    _log_handler = None

    log_file = Unicode().tag(config=True)

    @default('log_level')
    def _log_level_default(self):
        return logging.INFO

    @default('log_datefmt')
    def _log_datefmt_default(self):
        return "%Y-%m-%d %H:%M:%S"

    @default('log_format')
    def _log_format_default(self):
        return "[%(levelname)1.1s %(asctime)s.%(msecs).03d %(name)s %(module)s:%(lineno)d] %(message)s"

    extra_log_handlers = List(Instance(logging.Handler), config=True)
    extra_loggers = List(Any, default=[])

    @default('extra_log_handlers')
    def _extra_log_handlers(self):
        if self.log_file:
            handler = logging.handlers.WatchedFileHandler(self.log_file)
            return [handler]

        return []

    def init_logging(self):
        self.log.propagate = False

        # disable curl debug, which is TOO MUCH
        logging.getLogger('tornado.curl_httpclient').setLevel(
            max(self.log_level, logging.INFO)
        )

        _formatter = self._log_formatter_cls(
            fmt=self.log_format, datefmt=self.log_datefmt
        )

        for handler in self.extra_log_handlers:
            self.log.addHandler(handler)

        for handler in self.log.handlers:
            handler.setFormatter(_formatter)

        for logger in self.extra_loggers:
            if isinstance(logger, str):
                logger = logging.getLogger(logger)
            logger.propagate = True
            logger.parent = self.log
            logger.setLevel(self.log.level)
