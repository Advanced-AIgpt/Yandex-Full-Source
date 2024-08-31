#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import alice.tools.launch.runners as runners
import argparse
import logging
import os
import signal
import sys
import time

from library.python.vault_client import instances as YAV


class Settings(object):
    def __init__(self):
        self.base_port = 0
        self.servers = {}
        self.sorted_servers = ['redis', 'bass', 'vins', 'megamind']
        self.package_path = None
        self.log_dir = os.path.join(os.getcwd(), 'logs')
        self.additional_args_paths = {'bass': 'launch_configs/bass.txt',
                                      'megamind': 'launch_configs/megamind.txt',
                                      'vins': 'launch_configs/vins.txt'}
        self.configs = {
            'prod': {
                'bass': 'bass_configs/production_config.json',
                'megamind': 'megamind_configs/production/megamind.pb.txt',
                'redis': 'redis.conf.base',
                'vins': 'cit_configs'
            },
            'hamster': {
                'bass': 'bass_configs/hamster_config.json',
                'megamind': 'megamind_configs/hamster/megamind.pb.txt',
                'redis': 'redis.conf.base',
                'vins': 'cit_configs'
            },
            'rc': {
                'bass': 'bass_configs/rc_config.json',
                'megamind': 'megamind_configs/rc/megamind.pb.txt',
                'redis': 'redis.conf.base',
                'vins': 'cit_configs'
            },
            'local': {
                'bass': 'bass_configs/localhost_config.json',
                'megamind': 'megamind_configs/dev/megamind.pb.txt',
                'redis': 'redis.conf.base',
                'vins': 'cit_configs'
            }
        }
        self.custom_configs = {}

        self.run_mode = 'local'
        self.port_shifts = {'bass': 6,
                            'megamind': 0,
                            'redis': 8,
                            'vins': 5}
        self.pingers = {'bass': runners.BassServer.ping,
                        'megamind': runners.MegamindServer.ping,
                        'vins': runners.VinsServer.ping}
        self.logs_reloaders = {'bass': runners.BassServer.reload_logs,
                               'megamind': runners.MegamindServer.reload_logs,
                               'vins': runners.VinsServer.reload_logs}

    def get_config(self, server_name):
        if server_name in self.custom_configs:
            return self.custom_configs[server_name]

        return os.path.join(self.package_path, self.configs[self.run_mode][server_name])

    def get_port(self, server_name):
        return self.base_port + self.port_shifts[server_name]

    def get_url(self, server_name):
        return 'http://localhost:{}/'.format(self.get_port(server_name))

    def get_args(self, server_name):
        file_name = os.path.join(self.package_path, self.additional_args_paths[server_name])
        args = []
        if file_name is None:
            return args
        try:
            with open(file_name, 'r') as f:
                for arg in f.read().split():
                    args.append(arg)
        except FileNotFoundError:
            pass
        return args


def prepare_env(settings):
    if 'BASS_AUTH_TOKEN' in os.environ:
        kwargs = {'authorization': 'OAuth ' + os.environ['BASS_AUTH_TOKEN']}
    else:
        kwargs = {}
    if settings.run_mode == 'local':
        key = 'sec-01cnbk6vvm6mfrhdyzamjhm4cm'
    else:
        key = 'sec-01cxsqmp818f86wzv3rkshpctq'
    yav = YAV.Production(**kwargs)
    secrets = yav.get_version(key)['value']
    if not secrets:
        logging.info('Failed to get secrets from YAV. Shuting down')
        sys.exit()
    os.environ.update(secrets)
    os.environ['MONGO_PASSWORD'] = os.environ['VINS_MONGO_PASSWORD']


def create_bass(settings):
    name = 'bass'
    port = settings.get_port(name)
    bin_path = os.path.join(settings.package_path, 'bin', 'bass_server')
    config_path = settings.get_config(name)
    args = settings.get_args(name)
    settings.servers[name] = runners.BassServer(bin_path, settings.log_dir, config_path, port, settings.package_path, *args)


def create_vins(settings):
    name = 'vins'
    port = settings.get_port(name)
    bin_path = os.path.join(settings.package_path, 'run-vins.py')
    config_path = settings.get_config(name)
    bass_url = settings.get_url('bass')
    redis_port = settings.get_port('redis')
    args = settings.get_args(name)
    settings.servers[name] = runners.VinsServer(bin_path, settings.log_dir, config_path, port, settings.package_path, bass_url, redis_port, *args)


def create_megamind(settings):
    name = 'megamind'
    port = settings.get_port(name)
    bin_path = os.path.join(settings.package_path, 'bin', 'megamind_server')
    config_path = settings.get_config(name)
    bass_url = settings.get_url('bass')
    vins_url = settings.get_url('vins')
    args = settings.get_args(name)
    settings.servers[name] = runners.MegamindServer(bin_path, settings.log_dir, config_path, port, settings.package_path, vins_url, bass_url, *args)


def create_redis(settings):
    name = 'redis'
    port = settings.get_port(name)
    bin_path = os.path.join(settings.package_path, 'redis-server')
    config_path = settings.get_config(name)
    settings.servers[name] = runners.RedisServer(bin_path, settings.log_dir, config_path, port)


def create_lonely_megamind(settings):
    name = 'megamind'
    port = settings.get_port(name)
    bin_path = os.path.join(settings.package_path, 'bin', 'megamind_server')
    config_path = settings.get_config(name)
    args = settings.get_args(name)
    settings.servers[name] = runners.MegamindServer(bin_path, settings.log_dir, config_path, port, settings.package_path, None, None, *args)


def finalizer(settings):
    for server_name, server in settings.servers.items():
        server.stop()
    sys.exit()


def prepare(args):
    logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')
    settings = Settings()
    settings.base_port = int(args.port)

    if args.command in ['reload_logs', 'status']:
        return settings

    settings.run_mode = args.run_mode
    settings.package_path = args.package_path
    if args.log_dir:
        settings.log_dir = args.log_dir
    logging.info('Vins_package_path = {}'.format(settings.package_path))
    logging.info('Log dir = {}'.format(settings.log_dir))
    if not os.path.exists(settings.log_dir):
        os.mkdir(settings.log_dir)

    def handler(a, b):
        finalizer(settings)
    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)

    prepare_env(settings)

    if args.megamind_config is not None:
        settings.custom_configs['megamind'] = args.megamind_config

    if args.command == 'megamind_only':
        create_lonely_megamind(settings)
        return settings

    if args.bass_config is not None:
        settings.custom_configs['bass'] = args.bass_config

    create_bass(settings)
    create_vins(settings)
    create_megamind(settings)
    create_redis(settings)
    return settings


def wait_all(settings):
    for server in settings.servers.values():
        if server._server is not None:
            server._server.wait()


def start_servers(settings, servers):
    for server in settings.sorted_servers:
        if servers.count(server):
            settings.servers[server].start()


def check_servers_started(settings, servers):
    for server in servers:
        while not settings.servers[server].status():
            time.sleep(10)


def status(settings, server):
    url = settings.get_url(server)
    return settings.pingers[server](url)


def reload_logs(settings, server):
    url = settings.get_url(server)
    return settings.logs_reloaders[server](url)


def add_base_run_options(parser):
    parser.add_argument(
        '--port', '-p', dest='port', required=True,
        help='Base port'
    )
    parser.add_argument(
        '--package_path', '-w', dest='package_path',
        help='Path to vins_package',
        default=os.getcwd()
    )
    parser.add_argument(
        '--megamind-conf', dest='megamind_config',
        help='Path to custom megamind config',
        default=None
    )
    parser.add_argument(
        '--run-mode', default='local', dest='run_mode',
        choices=['prod', 'hamster', 'rc', 'local'],
        help='How to run MM, VINS, BASS'
    )
    parser.add_argument(
        '--log-dir', '-l', dest='log_dir',
        help='Path to custom log dir',
        default=None
    )


def add_run_options(parser):
    add_base_run_options(parser)
    parser.add_argument(
        '--bass', '-b', action='append_const',
        const='bass', help='Add bass',
        dest='servers'
    )
    parser.add_argument(
        '--vins', '-v', action='append_const',
        const='vins', help='Add vins',
        dest='servers'
    )
    parser.add_argument(
        '--megamind', '-m', action='append_const',
        const='megamind', help='Add megamind',
        dest='servers'
    )
    parser.add_argument(
        '--redis', '-r', action='append_const',
        const='redis', help='Add redis',
        dest='servers'
    )
    parser.add_argument(
        '--all', '-a', action='store_true',
        default=False, dest='all',
        help='Run all components'
    )
    parser.add_argument(
        '--bass-conf', dest='bass_config',
        help='Path to custom bass config',
        default=None
    )


def add_status_options(parser):
    parser.add_argument(
        'server', choices=['megamind', 'bass', 'vins'],
        help='Choose component'
    )
    parser.add_argument(
        '--port', '-p', dest='port', required=True,
        help='Base port'
    )


def add_reload_logs_options(parser):
    parser.add_argument(
        'server', choices=['megamind', 'bass', 'vins'],
        help='Choose component'
    )
    parser.add_argument(
        '--port', '-p', dest='port', required=True,
        help='Base port'
    )


def add_megamind_only_options(parser):
    add_base_run_options(parser)


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title='command', dest='command')

    run_parser = subparsers.add_parser('run', help='Use this mode to launch a couple of components')
    status_parser = subparsers.add_parser('status', help='Use this mode to find out status of ONLY ONE component')
    reload_logs_parser = subparsers.add_parser('reload_logs', help='Use this mode to reopen logs of ONLY ONE component')
    megamind_only_parser = subparsers.add_parser('megamind_only', help='Use this mode to launch lonely megamind')
    add_run_options(run_parser)
    add_status_options(status_parser)
    add_reload_logs_options(reload_logs_parser)
    add_megamind_only_options(megamind_only_parser)

    args = parser.parse_args()
    settings = prepare(args)

    if args.command == 'status':
        logging.info('Status of {}'.format(args.server))
        exit_code = 0
        if not status(settings, args.server):
            exit_code = 2
        sys.exit(exit_code)

    if args.command == 'megamind_only':
        try:
            start_servers(settings, ['megamind'])
            check_servers_started(settings, ['megamind'])
            logging.info("Successful start")
            wait_all(settings)
        except Exception as ex:
            logging.info('Exception caught, finalizing...')
            logging.info(ex)
            finalizer(settings)

    if args.command == 'reload_logs':
        logging.info('Reloading {} logs'.format(args.server))
        exit_code = 0
        if not reload_logs(settings, args.server):
            exit_code = 2
        sys.exit(exit_code)

    try:
        if args.all:
            args.servers = settings.sorted_servers
        start_servers(settings, args.servers)
        check_servers_started(settings, args.servers)
        logging.info("Successful start")
        wait_all(settings)
    except Exception as ex:
        logging.info('Exception caught, finalizing...')
        logging.info(ex)
        finalizer(settings)

if __name__ == '__main__':
    main()
