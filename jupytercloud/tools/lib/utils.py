# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import sys
import logging
import coloredlogs
import tqdm
import sentry_sdk

from .environment import coerce_environment

LOGGERS = ('jupytercloud', 'paramiko')
FORMAT = "%(asctime)s.%(msecs)d %(levelname)s %(name)s: %(message)s"


class TqdmStream(object):
    def __init__(self, stream):
        self.stream = stream

    def write(self, msg):
        tqdm.tqdm.write(msg, file=self.stream, end='')

    def isatty(self):
        try:
            return self.stream.isatty()
        except Exception:
            return False


def setup_logging(program_name, verbose_level=0, logs_dir=None):
    assert program_name.startswith('jupytercloud.')

    formatter = logging.Formatter(FORMAT)

    loggers = [logging.getLogger(logger) for logger in LOGGERS]

    filename = program_name + '.log'
    if logs_dir:
        filename = '{}/{}'.format(logs_dir, filename)

    file_handler = logging.FileHandler(filename=filename, mode='w')
    file_handler.setFormatter(formatter)
    file_handler.setLevel(logging.DEBUG)

    logging_level = max((30 - verbose_level * 10, 1))

    for logger in loggers:
        # change INFO level to WARNING for paramiko if
        # verbose level is INFO to prevent excessive logging
        if logger.name == 'paramiko' and verbose_level == 1:
            logging_level = logging.WARNING

        logger.addHandler(file_handler)

        coloredlogs.install(
            level=logging_level,
            logger=logger,
            fmt=FORMAT,
            stream=TqdmStream(sys.stderr),
        )

        # this is called after coloredlogs on purpose
        logger.setLevel(logging.DEBUG)

    return logging.getLogger(program_name)


def setup_sentry(app_name, environment=None):
    environment = coerce_environment(environment)

    sentry_sdk.init(
        dsn=environment.sentr_dsn,
        ca_certs=environment.ssl_root_cert,
        environment=environment.name,
        dist=app_name,
    )

    with sentry_sdk.configure_scope() as scope:
        scope.set_tag('application', app_name)
