# -*- coding: utf-8 -*-

import argparse
import os

from library.python.vault_client import instances as YAV


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-r', '--rename', action='append', help='Rename variable from YAV (i.e. -r TEST_AUTH_TOKEN=AUTH_TOKEN)')
    parser.add_argument('-t', '--token-env-name', default='AUTH_TOKEN', help='Use this name to obtain the token from ENV')
    parser.add_argument('key_id', nargs=1, help='Secret Head ID from yav.yandex-team.ru')
    parser.add_argument('run_cmd', nargs='+', help='Command and its args to run')
    return parser.parse_args()


def main(args):
    yav = YAV.Production(authorization='OAuth ' + os.environ[args.token_env_name])
    envdata = yav.get_version(args.key_id[0])['value']
    args.rename = args.rename or []
    for item in args.rename:
        print(item)
        name, new_name = item.split('=', 1)
        value = envdata.pop(name, None)
        if value is not None:
            envdata[new_name] = value
    envdata.update(os.environ)
    os.execvpe(args.run_cmd[0], args.run_cmd, envdata)


if __name__ == '__main__':
    main(parse_args())
