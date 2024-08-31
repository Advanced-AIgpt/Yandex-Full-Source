import argparse
import asyncio
import logging
import os
import signal
from pathlib import Path

import alice.library.python.server_runner.resource_environment as resource_environment
from alice.library.python.utils.arcadia import arcadia_path
from library.python.vault_client.instances import Production as VaultClient

# https://yav.yandex-team.ru/secret/sec-01cnbk6vvm6mfrhdyzamjhm4cm
SECRET = 'sec-01cnbk6vvm6mfrhdyzamjhm4cm'


def _get_env_from_yav(secret):
    try:
        return VaultClient().get_version(secret)['value']
    except BaseException as e:
        logging.warn(f'Exception: {e}')
        logging.warn('Most likely you are not a part of abc:bassdevelopers')
        logging.warn(f'Try opening page https://yav.yandex-team.ru/secret/{secret}')
        return {}


def _add_unified_agent_args(parser, ua_config=None, ua_logs=None, ua_uri=None):
    group = parser.add_argument_group('Unified agent', 'Unified agent group params')
    group.add_argument(
        '--ua-binary',
        default=arcadia_path('logbroker', 'unified_agent', 'bin', 'unified_agent'),
        type=Path,
        help='Path to unified agent binary',
    )
    parser.add_argument(
        '--ua-config',
        default=ua_config,
        type=Path,
        help='Path to unified agent config',
    )
    parser.add_argument(
        '--ua-logs',
        default=ua_logs,
        type=Path,
        help='Path to directory with sandbox resources',
    )
    parser.add_argument(
        '--ua-uri',
        default=ua_uri,
        help='Valuef for "rtlog-unified-agent-uri" param',
    )
    parser.add_argument(
        '--ua-log-file',
        default=ua_logs / 'unified_agent_backend.err' if ua_logs else None,
        type=Path,
        help='Valuef for "rtlog-unified-agent-log-file" param',
    )


def parse_known_args(source_dir, default_server, logs=None, args=None, **kwargs):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--local',
        dest='use_local',
        action='store_true',
        help='Use local copy of vins_package.json',
    )
    parser.add_argument(
        '--gdb',
        dest='use_gdb',
        action='store_true',
        help='Use ya tool gdb',
    )
    parser.add_argument(
        '--server',
        default=default_server,
        type=Path,
        help='Path to server binary',
    )
    parser.add_argument(
        '--logs',
        default=logs,
        type=Path,
        help='Path to log directory (stdout, stderr)',
    )
    parser.add_argument(
        '--arcadia-dir',
        default=arcadia_path(''),
        type=Path,
        help='Arcadia root directory',
    )
    parser.add_argument(
        '--source-dir',
        default=source_dir,
        help='Path to directory with sandbox resources',
    )
    parser.add_argument(
        '--vins-package-abs-path',
        help='Path to vins_package.json if svn_co or sandbox_fetcher is unavailable',
    )

    parser.add_argument(
        '--run-unified-agent',
        action='store_true',
        help='Runs unified agent alongside, allows to send local logs to SETrace',
    )
    _add_unified_agent_args(parser, **kwargs)

    return parser.parse_known_args(args)


async def _run_service(argv, stdout=None, stderr=None):
    service_name = Path(argv[0]).name
    env = _get_env_from_yav(SECRET)
    env.update(os.environ)
    stdout_pipe = stdout.open('w') if stdout else None
    stderr_pipe = stderr.open('w') if stderr else None
    try:
        logging.info(f'Starting {service_name}')
        logging.info(f'Running command: {" ".join(argv)}')
        proc = await asyncio.create_subprocess_exec(
            *argv, env=env, stdout=stdout_pipe, stderr=stderr_pipe,
        )
        logging.info(f'Started server {service_name}, pid: {proc.pid}')
        await proc.wait()
    except asyncio.CancelledError:
        proc.send_signal(signal.SIGINT)
        logging.info(f'Canceled {service_name}')
        await proc.wait()
    finally:
        if stdout_pipe:
            stdout_pipe.close()
        if stderr_pipe:
            stderr_pipe.close()
        logging.info(f'Finished {service_name}')


async def run_unified_agent(args, argv):
    if not args.ua_binary or not args.ua_binary.is_file():
        logging.error('Cannot find unified_agent binary. Please run `ya make arcadia/logbroker/unified_agent/bin`')
        return

    args.ua_logs.mkdir(parents=True, exist_ok=True)
    os.environ['BASS_DEV_TVM_SECRET'] = _get_env_from_yav(SECRET)['TVM2_SECRET']
    ua_argv = [
        str(args.ua_binary),
        '--config', str(args.ua_config),
        '--log-file', str(args.ua_logs / 'unified_agent.err'),
    ]

    await _run_service(ua_argv)


async def run(args, argv, sandbox_resources={}, map_to_argv=None):
    if not args.server or not args.server.is_file():
        raise Exception(f'Server binary {args.server} not found, use either ARCADIA_ROOT env or --server cmd line option')

    argv = [str(args.server)] + argv
    if args.use_gdb:
        ya_path = str(args.arcadia_dir / 'ya')
        argv = [ya_path, 'tool', 'gdb', '--args'] + argv

    if args.run_unified_agent:
        argv.extend(['--rtlog-unified-agent-uri', args.ua_uri])
        argv.extend(['--rtlog-unified-agent-log-file', str(args.ua_log_file)])

    resources = set([k for k, v in sandbox_resources.items() if v not in argv])
    for res_type, res_path in resource_environment.download(args, resources):
        arg_key, arg_value = map_to_argv(res_type, res_path)
        argv.extend([arg_key, arg_value])

    resources = set(sandbox_resources.keys()) - resources
    for res_type in resources:
        index = argv.index(sandbox_resources[res_type])
        argv[index], argv[index + 1] = map_to_argv(res_type, argv[index + 1])

    if args.logs:
        args.logs.mkdir(parents=True, exist_ok=True)
    stdout = args.logs / f'{args.server.name}.out' if args.logs else None
    stderr = args.logs / f'{args.server.name}.err' if args.logs else None

    await _run_service(argv, stdout, stderr)
