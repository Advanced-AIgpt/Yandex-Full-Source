import argparse
import asyncio
import logging

from aiohttp import web
from alice.tools.version_alerts.lib.common import make_logger
from alice.tools.version_alerts.lib.responder import Responder

from alice.tools.quota_monitor.lib.collector import Collector

LOGLEVEL = logging.INFO

log = make_logger('main')


async def start_background_tasks(app_ref: web.Application):
    app_ref['data_collector']['worker'] = Collector(app_ref['data_collector']['ttl'])
    app_ref['data_collector']['task'] = asyncio.create_task(app_ref['data_collector']['worker'].run())


async def cleanup_background_tasks(app_ref: web.Application):
    log.error("Shutting down")
    app_ref['data_collector']['task'].cancel()
    await app_ref['data_collector']['task']


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument('--wait', help='Pause between checks (seconds)', default=3600, type=int)
    parser.add_argument('--config', help='Path to services config json', default='config.json', type=str)
    parser.add_argument('--host', help='Host to listen', default='::', type=str)
    parser.add_argument('--port', help='Port to liten', default=8200, type=int)
    parser.add_argument('--debug', help='Set log level to debug', default=False, action='store_true')
    return parser


def main(wait_time: int = 3600, host: str = '127.0.0.1', port: int = 8080):
    responder = Responder()
    app = responder.app()
    app['data_collector'] = {
        'ttl': wait_time,
        'config_path': 'data.json',
    }
    app.on_startup.append(start_background_tasks)
    app.on_cleanup.append(cleanup_background_tasks)
    try:
        web.run_app(app, host=host, port=port)
    except PermissionError as e:
        log.error(f"Can't start app on {host}:{port}")
        log.error(e)


if __name__ == '__main__':
    argument_parser = create_parser()
    arguments = argument_parser.parse_args()
    loglevel = logging.DEBUG if arguments.debug else logging.INFO
    logging.basicConfig(level=loglevel)
    main(wait_time=arguments.wait, host=arguments.host, port=arguments.port)
