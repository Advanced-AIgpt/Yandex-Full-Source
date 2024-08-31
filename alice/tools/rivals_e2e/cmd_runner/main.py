# -*- coding: utf-8 -*-
import argparse
import datetime
import requests
import subprocess
import sys
import logging
import time
import json


def initialize_logging(name=None):
    log = logging.getLogger(name)
    log.setLevel(logging.INFO)
    _stream_handler = logging.StreamHandler()
    _stream_handler.setLevel(logging.DEBUG)
    _stream_handler.setFormatter(logging.Formatter(
        u'%(filename)s[LINE:%(lineno)d]# %(levelname)-8s [%(asctime)s]  %(message)s'
    ))
    log.addHandler(_stream_handler)
    log.propagate = False
    return log


logger = initialize_logging(__name__)

DEFAULT_DEVICE_FILTER = {
    'status': 'READY',
    'osType': 'ANDROID',
    'deviceType': 'PHONE',
    'connectivityState': True,
    'capabilityTags': ['Root'],
    'group': ['ALICE']
}

DEFAULT_OCCUPY_HOURS = 3

DEFAULT_RETRY_COUNT = 3

DEFAULT_DEVICE_OPTIONS = {
    'devicesCount': 1
}

DEFAULT_COMMANDS = {
    'commands': [
        ['adb', 'shell', 'pm', 'list', 'packages', '-3']
    ]
}

headers = None
device_options = DEFAULT_DEVICE_OPTIONS
connection = None


def timestamp_delta(hours):
    relative_time = datetime.datetime.now() + datetime.timedelta(hours=hours)
    return int(relative_time.timestamp() * 1000)


def occupy_some_device(device_filter, device_options):
    return http_post(
        'https://kolhoz.yandex-team.ru/api/public/v1/devices/occupy',
        {'filter': device_filter, 'options': device_options}
    ).json()


def enable_remote_connect(device_id):
    return http_post(
        f'https://kolhoz.yandex-team.ru/api/public/v1/devices/{device_id}/connect'
    ).json()


def release_device(device_id):
    return http_post(
        f'https://kolhoz.yandex-team.ru/api/public/v1/devices/{device_id}/release'
    )


def make_http_headers(token):
    return {
        'Authorization': f'OAuth {token}',
        'Content-Type': 'application/json',
    }


def http_post(url, json=None):
    if headers is None:
        raise Exception('HTTP request headers must be set')

    request = f'POST {url}'
    logger.info(request)
    response = requests.post(url=url, headers=headers, json=json)
    if response.status_code == 200:
        return response
    else:
        raise Exception(f'Unexpected status-code {response.status_code} during http-request: {request}')


def execute_shell_cmd(cmd, retry_attempts=0):
    global connection
    logger.info(f'Executing: {cmd}')
    retry_count = DEFAULT_RETRY_COUNT
    can_retry = True if cmd[-1] == '__try_retry__' else False
    try:
        if can_retry:
            subprocess.check_call(cmd[:-1], stdout=sys.stdout, stderr=subprocess.STDOUT)
        else:
            subprocess.check_call(cmd, stdout=sys.stdout, stderr=subprocess.STDOUT)
    except:
        retry_attempts += 1
        if retry_attempts <= retry_count and can_retry:
            logger.error(f'Failed to execute: {cmd}, retrying, attempt: {retry_attempts} with adb reconnect')
            execute_shell_cmd(['adb', 'connect', f"{connection['hostname']}:{connection['port']}"])
            time.sleep(2)
            execute_shell_cmd(cmd, retry_attempts)
        else:
            raise
    return cmd


def with_adb_connect(hostname, port, action):
    host_port = f'{hostname}:{port}'
    try:
        execute_shell_cmd(['adb', 'connect', host_port])
        time.sleep(2)  # wait device authorization
        stdout, stderr = execute_shell_cmd(['adb', 'devices'])
        if 'offline' not in str(stdout):
            action(host_port)
        else:
            raise Exception('device offline after adb-connect')
    finally:
        execute_shell_cmd(['adb', 'disconnect', host_port])


def with_enable_remote_connect(device_id, action):
    global connection
    connection = enable_remote_connect(device_id)
    if connection['enabled']:
        logger.info(f'Remote connect successfully enabled on device: {device_id}')
        action(connection['hostname'], connection['port'])
    else:
        raise Exception(f'enabled = false, after enable_remote connect {device_id}')


def with_occupied_device(action):
    global device_options
    global device_id
    device = occupy_some_device(DEFAULT_DEVICE_FILTER, device_options)
    if len(device['results']) > 0:
        device_id = device['results'][0]['device']['id']
        model_name = device['results'][0]['device']['modelName']
        try:
            logger.info(f'Device successfully occupied: {device_id} / {model_name}')
            action(device_id)
        finally:
            release_device(device_id)
            logger.info(f'Device successfully released: {device_id} / {model_name}')
    else:
        raise Exception('no devices after occupy-request')


def execute_all(device_id, commands):
    for cmd in commands:
        execute_shell_cmd(cmd)


def parse_args(args=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--kolhoz_token',
        type=str,
        help='Kolhoz OAuth Token',
        required=True)
    parser.add_argument(
        '--occupy_hours',
        type=int,
        default=DEFAULT_OCCUPY_HOURS,
        help='Hours. How long device will be booked')
    parser.add_argument(
        '--commands',
        type=json.loads,
        default=DEFAULT_COMMANDS,
        help=f'Dict with lists of commands. Example: \n {json.dumps(DEFAULT_COMMANDS)}')
    return parser.parse_args(args)


def main():
    args = parse_args()

    global headers
    headers = make_http_headers(args.kolhoz_token)
    commands = args.commands['commands']

    global device_options
    device_options['toTs'] = timestamp_delta(args.occupy_hours)

    with_occupied_device(lambda device_id: (
        with_enable_remote_connect(device_id, lambda hostname, port: (
            with_adb_connect(hostname, port, lambda _device_id: (
                execute_all(_device_id, commands)
            ))
        ))
    ))


if __name__ == '__main__':
    main()
