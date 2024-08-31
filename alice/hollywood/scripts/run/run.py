import argparse
import asyncio
import logging
import os
from os.path import expanduser

import alice.hollywood.library.config.config_pb2 as config_proto
import alice.library.python.utils as utils
import coloredlogs
from alice.library.python import server_runner
from alice.library.python.utils.arcadia import arcadia_path


def parse_shard_args(argv=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--shard_name',
                        default='all',
                        dest='shard_name',
                        help='Shard name')
    return parser.parse_known_args(argv)


def parse_known_args(argv, shard_path):
    config_path = shard_path
    if 'dev' in os.listdir(config_path):
        config_path = config_path / 'dev'
    elif 'prod' in os.listdir(config_path):
        config_path = config_path / 'prod'
    else:
        raise Exception(f"Cannot find directories 'dev' or 'prod' in shard '{config_path}'")

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', '--config',
                        default=str(config_path / 'hollywood.pb.txt'),
                        help='Hollywood config')
    return parser.parse_known_args(argv)


def parse_paths(argv, hollywood_dir, config_path):
    config = utils.load_file(config_path, config_proto.TConfig())

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--fast-data-path',
                        default=str(hollywood_dir / config.FastDataPath),
                        help='Hollywood FastData path')
    parser.add_argument('--scenario-resources-path',
                        default=str(hollywood_dir / config.ScenarioResourcesPath),
                        help='Hollywood scenario resources path')
    parser.add_argument('--common-resources-path',
                        default=str(hollywood_dir / config.CommonResourcesPath),
                        help='Path for resources to which all scenarios shall have access')
    parser.add_argument('--hw-services-resources-path',
                        default=str(hollywood_dir / config.HwServicesResourcesPath),
                        help='Path for resources to which all hw services shall have access')
    return parser.parse_known_args(argv)


@server_runner.async_loop
async def run():
    hollywood_dir = arcadia_path('alice', 'hollywood')
    ua_log_path = hollywood_dir / 'logs'
    shard_args, argv = parse_shard_args()

    shard_path = hollywood_dir / 'shards' / shard_args.shard_name
    args, argv = server_runner.parse_known_args(
        source_dir=os.path.join(expanduser('~'), '.ya', 'hollywood_data'),
        default_server=shard_path / 'server' / 'hollywood_server',
        ua_config=hollywood_dir / 'scripts' / 'nanny_files' / 'unified_agent_config.yaml',
        ua_uri='localhost:12384',
        ua_logs=ua_log_path,
        args=argv,
    )

    hollywood_args, argv = parse_known_args(argv, shard_path)
    paths, argv = parse_paths(argv, hollywood_dir, hollywood_args.config)

    processes = []
    if args.run_unified_agent:
        processes.append(asyncio.create_task(
            server_runner.run_unified_agent(args, argv)
        ))
        await asyncio.sleep(0.1)

    argv = [
        '-c', hollywood_args.config,
        '--fast-data-path', paths.fast_data_path,
        '--scenario-resources-path', paths.scenario_resources_path,
        '--common-resources-path', paths.common_resources_path,
        '--hw-services-resources-path', paths.hw_services_resources_path,
    ] + argv
    processes.append(server_runner.run(args, argv))

    return processes


def main():
    logging.basicConfig(level=logging.INFO)
    coloredlogs.install(logging.INFO)
    asyncio.run(run())
