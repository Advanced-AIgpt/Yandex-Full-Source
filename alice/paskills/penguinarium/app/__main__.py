import gunicorn.app.base
from gunicorn import glogging
from aiohttp import web
import asyncio

import logging
from pythonjsonlogger import jsonlogger
from datetime import datetime
import argparse
from functools import partial
from typing import Dict

from alice.paskills.penguinarium.app.config import PenguinariumConfig
from alice.paskills.penguinarium.init_app import init_app


class CustomJsonFormatter(jsonlogger.JsonFormatter):
    def add_fields(self, log_record, record, message_dict):
        super(CustomJsonFormatter, self).add_fields(log_record, record, message_dict)
        if not log_record.get('timestamp'):
            dt = datetime.utcfromtimestamp(record.created)
            log_record['timestamp'] = dt.strftime('%Y-%m-%dT%H:%M:%S.%f')
        if log_record.get('level'):
            log_record['level'] = log_record['level'].upper()
        else:
            log_record['level'] = record.levelname


json_formatter = CustomJsonFormatter('%(process)s %(timestamp)s %(level)s %(name)s %(message)s')


class AsyncStreamHandler(logging.StreamHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def _get_loop(self):
        try:
            return asyncio.get_running_loop()
        except RuntimeError:
            pass

        return None

    def emit(self, record):
        loop = self._get_loop()
        if loop is None or loop.is_closed():
            super().emit(record)
        else:
            loop.call_soon(partial(super().emit, record))


class CustomGLogger(glogging.Logger):
    """Custom logger for Gunicorn log messages."""

    def _set_async_handler(self, log, fmt, stream=None):
        # remove previous gunicorn log handler
        h = self._get_gunicorn_handler(log)
        if h:
            log.handlers.remove(h)

        h = AsyncStreamHandler(stream)
        h.setFormatter(fmt)
        h._gunicorn = True
        log.addHandler(h)

    def setup(self, cfg):
        """Configure Gunicorn application logging configuration."""
        super().setup(cfg)

        self._set_async_handler(self.access_log, json_formatter)
        self._set_async_handler(self.error_log, json_formatter)


class GunicornApp(gunicorn.app.base.BaseApplication):
    def __init__(self, app: web.Application) -> None:
        self._app = app
        self._config = app['config']
        super().__init__()

    def load_config(self) -> None:
        self.cfg.set('workers', self._config['server']['workers'])
        host, port = self._config['server']['host'], self._config['server']['port']
        self.cfg.set('bind', f'{host}:{port}')
        self.cfg.set('timeout', self._config['server']['timeout'])
        self.cfg.set('worker_class', 'aiohttp.worker.GunicornUVLoopWebWorker')
        self.cfg.set('accesslog', '-')
        self.cfg.set('logger_class', CustomGLogger)

    def load(self) -> web.Application:
        return self.app

    @property
    def app(self) -> web.Application:
        return self._app


def init_logging(config: Dict) -> None:
    ydb_logger = logging.getLogger('ydb')
    ydb_logger.setLevel(config['kikimr_level'])

    logger = logging.getLogger()
    level = config['common_level']

    logger.setLevel(level)
    handler = AsyncStreamHandler()
    handler.setLevel(level)
    handler.setFormatter(json_formatter)
    logger.addHandler(handler)


def setup_argparser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-p',
        '--path_to_config',
        type=str,
        default='configs/debug_local/app.yaml',
        help='Path to config yaml file',
    )
    parser.add_argument(
        '-w',
        '--workers',
        type=int,
        help='Gunicorn workers number',
    )

    return parser


def main() -> None:
    parser = setup_argparser()
    cli_args = parser.parse_args()
    config = PenguinariumConfig(cli_args.path_to_config)

    if cli_args.workers is not None:
        config['server']['workers'] = cli_args.workers

    init_logging(config['logging'])

    gapp = GunicornApp(init_app(config))
    gapp.run()


if __name__ == '__main__':
    main()
