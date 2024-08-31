import asyncio
import logging
import os
from os.path import expanduser

import coloredlogs
from alice.library.python import server_runner
from alice.library.python.utils.arcadia import arcadia_path


SANDBOX_RESOURCES = {
    'GEODATA6BIN_STABLE': '--geobase-path',
}
ARGV = {
    'GEODATA6BIN_STABLE': 'ENV_GEOBASE_PATH={}',
}


def map_to_server_argv(res_type, res_path):
    return '-V', ARGV[res_type].format(res_path)


@server_runner.async_loop
async def run():
    bass_dir = arcadia_path('alice', 'bass')
    default_server = None
    if bass_dir:
        default_server = bass_dir / 'bin' / 'bass_server'

    args, argv = server_runner.parse_known_args(
        source_dir=os.path.join(expanduser('~'), '.ya', 'megamind_data'),
        default_server=default_server,
    )

    return [server_runner.run(args, argv, SANDBOX_RESOURCES, map_to_server_argv)]


def main():
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(logging.INFO)
    asyncio.run(run())
