import argparse
import asyncio
import json
import logging
import os
import random
from importlib import import_module
from time import sleep
from typing import Dict

from aiohttp import web

from alice.tools.version_alerts.lib.collector import Collector
from alice.tools.version_alerts.lib.common import make_logger, cleanup_background_tasks
from alice.tools.version_alerts.lib.responder import Responder
from alice.tools.version_alerts.lib.yasm import send_dashboards

MAX_SLEEP_BEFORE_START = 15
LOGLEVEL = logging.INFO

log = make_logger('main')
MY_PATH = 'alice.tools.version_alerts'
ACCEPTED_CONFIG_KEYS = (
    'url', 'token', 'beta_for', 'params', 'timeout', 'parser', 'check_resources', 'check_resources_only')


async def start_background_tasks(app_ref: web.Application):
    log.info(f'Starting background collector with refresh time {app_ref["data_collector"]["ttl"]}')
    config = read_config(app_ref['data_collector']['config_path'])
    collector = Collector(app_ref['data_collector']['ttl'], config)
    app_ref['data_collector']['worker'] = collector
    app_ref['data_collector']['task'] = asyncio.create_task(app_ref['data_collector']['worker'].run())
    if app_ref['data_collector']['dashboard']:
        await send_dashboards(config)


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument('--wait', help='Pause between checks (seconds)', default=60, type=int)
    parser.add_argument('--config', help='Path to services config json', default='config.json', type=str)
    parser.add_argument('--host', help='Host to listen', default='::', type=str)
    parser.add_argument('--port', help='Port to liten', default=8200, type=int)
    parser.add_argument('--dashboard', help='YASM Dashboard id', type=str, default=None)
    parser.add_argument('--debug', help='Set log level to debug', default=False, action='store_true')
    return parser


def read_config(config_file: str) -> Dict:
    default = dict.fromkeys(ACCEPTED_CONFIG_KEYS, None)

    try:
        with open(config_file) as f:
            config_file_dict = json.load(f)
    except FileNotFoundError:
        log.error(f'Config not found at {config_file}')
        quit(1)
    except json.decoder.JSONDecodeError as e:
        log.error(f'Can\'t parse json: {e}')
        quit(1)
    except Exception as e:
        log.exception(e)
        quit(1)

    service_dict = {}
    for service in config_file_dict:
        if service not in service_dict:
            service_dict[service] = default.copy()
            service_dict[service].update(config_file_dict[service])
        if 'token' in config_file_dict[service]:
            service_dict[service]['token'] = os.environ.get(config_file_dict[service]['token'])
        if 'parser' in config_file_dict[service]:
            service_dict[service]['parser'] = getattr(
                import_module(f"{MY_PATH}.parsers.{config_file_dict[service]['parser']}"),
                config_file_dict[service]['parser'])()

    return service_dict


def main(wait_time: int, host: str, port: int, config_path: str, dashboard: str):
    responder = Responder()

    app = responder.app()
    app['data_collector'] = {
        'ttl': wait_time,
        'config_path': config_path,
        'dashboard': dashboard
    }
    app.on_startup.append(start_background_tasks)
    app.on_cleanup.append(cleanup_background_tasks)

    try:
        web.run_app(app, host=host, port=port, access_log=None)
    except PermissionError as e:
        log.exception(f"Can't start app on {host}:{port}")
        log.exception(e)


if __name__ == '__main__':
    arguments_parser = create_parser()
    arguments = arguments_parser.parse_args()
    log.level = logging.DEBUG if arguments.debug else logging.INFO
    sleep_time = random.randint(0, MAX_SLEEP_BEFORE_START)
    log.info(f'Sleeping for {sleep_time} seconds')
    sleep(sleep_time)
    main(arguments.wait, arguments.host, arguments.port, arguments.config, arguments.dashboard)
