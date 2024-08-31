#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# If you want to change this code, don't do this.
# Read README.md for more information

import argparse
import json
import logging
import os
import subprocess


# https://yav.yandex-team.ru/secret/sec-01cnbk6vvm6mfrhdyzamjhm4cm
SECRET = 'sec-01cnbk6vvm6mfrhdyzamjhm4cm'

BASS_DIR = os.path.realpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..'))
YA = os.path.realpath(os.path.join(BASS_DIR, '..', '..', 'ya'))

GEODATA6BIN_RESOURCE_ID = 3069941764
GEODATA6BIN_RESOURSE_TYPE = 'GEODATA6BIN_STABLE'
GEODATA6BIN_FILENAME = 'geodata6.bin'


def is_downloaded(res_type, res_id, source_path):
    file_path = os.path.join(source_path, res_type)
    if not os.path.exists(file_path):
        return False

    with open(file_path) as stream:
        saved_res_id = stream.read()

    return saved_res_id == str(res_id)


def save_dowloaded_res_id(res_type, res_id, source_path):
    file_path = os.path.join(source_path, res_type)
    with open(file_path, 'w') as stream:
        stream.write(str(res_id))


def download_resource(source_path, res_type, res_id, filename):
    file_path = os.path.join(source_path, filename)
    if is_downloaded(res_type, res_id, source_path) and os.path.exists(file_path):
        return file_path

    if os.path.exists(file_path):
        os.remove(file_path)

    logging.info('Downloading resource {} of type {} into {}'.format(res_id, res_type, source_path))
    subprocess.check_call(['sky', 'get', '-D', 'Hardlink', '-wup', '-d', source_path, 'sbr:{}'.format(res_id)])

    save_dowloaded_res_id(res_type, res_id, source_path)


def download_geobase(source_path):
    return download_resource(source_path, GEODATA6BIN_RESOURSE_TYPE, GEODATA6BIN_RESOURCE_ID, GEODATA6BIN_FILENAME)


def get_version(version):
    cmd = [YA, 'vault', 'get', 'version', '--compact', '-j', version]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = proc.communicate()
    return out.decode('utf-8')


def parse_known_args(default_server, args=None):
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--local', dest='use_local', action='store_true', help='Use local copy of vins_package.json')
    parser.add_argument('--gdb', dest='use_gdb', action='store_true', help='Use ya tool gdb')
    parser.add_argument('--server', default=default_server, help='Path to server binary')
    parser.add_argument('--geobase-path', help='Path to geodata6.bin')
    parser.add_argument('--no-bass-secrets', action='store_true', help='Dont use YAV secrets')
    return parser.parse_known_args(args)


def main():
    args, argv = parse_known_args(
        default_server=os.path.join(BASS_DIR, 'bin', 'bass_server')
    )

    argv = [args.server] + argv
    if args.use_gdb:
        argv = [YA, 'tool', 'gdb', '--args'] + argv

    source_path = os.path.join(os.environ['HOME'], '.ya', 'bass_data')
    geobase_path = args.geobase_path or download_geobase(source_path)
    argv.extend(['-V', 'ENV_GEOBASE_PATH={}'.format(geobase_path)])

    env = os.environ
    if not args.no_bass_secrets:
        secrets = get_version(SECRET)
        if not secrets:
            raise Exception('Failed to get secrets from YAV')
        env.update(json.loads(secrets)['value'])

    os.execvpe(file=args.server, args=argv, env=env)


if __name__ == '__main__':
    main()
