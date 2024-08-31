import functools
import logging
import sys
import asyncio

from aiohttp import web

LOG_STRING_FORMAT = '%(asctime)s - %(name)s - %(levelname)s - %(message)s'


def make_logger(name: str) -> logging.Logger:
    log = logging.getLogger(name)
    log.propagate = False
    formatter = logging.Formatter(LOG_STRING_FORMAT)

    stdout_handler = logging.StreamHandler(sys.stdout)
    stdout_handler.formatter = formatter
    stdout_handler.addFilter(lambda record: record.levelno <= logging.INFO)

    stderr_handler = logging.StreamHandler()
    stderr_handler.addFilter(lambda record: record.levelno > logging.INFO)
    stderr_handler.formatter = formatter

    log.addHandler(stdout_handler)
    log.addHandler(stderr_handler)

    return log


async def cleanup_background_tasks(app_ref: web.Application):
    log = logging.getLogger('main')
    log.error("Shutting down")
    app_ref['data_collector']['task'].cancel()
    await app_ref['data_collector']['task']


async def flatten(list_of_lists):
    return [x for items in list_of_lists for x in items]


def run_in_executor(f):
    @functools.wraps(f)
    def inner(*args, **kwargs):
        loop = asyncio.get_running_loop()
        return loop.run_in_executor(None, functools.partial(f, *args, **kwargs))

    return inner
