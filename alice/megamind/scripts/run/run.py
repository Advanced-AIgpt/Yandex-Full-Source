import asyncio
import logging
import os
from os.path import expanduser

import coloredlogs
from alice.library.python import server_runner
from alice.library.python.utils.arcadia import arcadia_path


SANDBOX_RESOURCES = {
    'FORMULAS_RESOURCE': '--formulas-path',
    'GEODATA6BIN_STABLE': '--geobase-path',
    'PARTIAL_PRECLASSIFICATION_MODEL': '--partial-pre-classification-model-path',
}


def map_to_server_argv(res_type, res_path):
    return SANDBOX_RESOURCES[res_type], res_path


@server_runner.async_loop
async def run():
    megamind_dir = arcadia_path('alice', 'megamind')
    default_server = None
    ua_log_path = None
    ua_config = None
    if megamind_dir:
        default_server = megamind_dir / 'server' / 'megamind_server'
        ua_log_path = megamind_dir / 'logs'
        ua_config = megamind_dir / 'deploy' / 'nanny' / 'dev_unified_agent_config.yaml'

    args, argv = server_runner.parse_known_args(
        source_dir=os.path.join(expanduser('~'), '.ya', 'megamind_data'),
        default_server=default_server,
        ua_config=ua_config,
        ua_logs=ua_log_path,
        ua_uri='localhost:12387',
    )

    processes = []
    if args.run_unified_agent:
        processes.append(asyncio.create_task(
            server_runner.run_unified_agent(args, argv)
        ))
        await asyncio.sleep(0.1)

    logging.info('Starting server: %s, args: %s, argv: %s', args.server, args, argv)
    processes.append(server_runner.run(
        args, argv, SANDBOX_RESOURCES, map_to_server_argv,
    ))

    return processes


def main():
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(logging.INFO)
    asyncio.run(run())
