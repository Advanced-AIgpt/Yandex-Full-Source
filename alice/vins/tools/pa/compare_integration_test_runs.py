#!/usr/bin/env python2
# -*- coding: utf-8 -*-
from __future__ import print_function
import argparse
import json
import sys


def load(path):
    with open(path) as fd:
        return json.load(fd)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('STABLE', help='Stable run json path')
    parser.add_argument('TESTING', help='Testing run json path')
    parser.add_argument(
        '--all', dest='should_print_all', action='store_true', default=False,
        help='Print all changes, including fixed test cases'
    )
    return parser.parse_args()


def main():
    args = parse_args()
    data_stable = load(args.STABLE)
    data_testing = load(args.TESTING)

    for test_stable, test_testing in zip(data_stable['tests'], data_testing['tests']):
        if test_stable['success'] and test_testing['success']:
            continue
        dialog = []
        for turn_stable, turn in zip(test_stable['turns'], test_testing['turns']):
            dialog.append(u'User:  {0}'.format(turn['request']))
            dialog.append(u'Alice: {0}'.format(turn['response']))
            if turn_stable['success'] == turn['success']:
                continue
            if turn['success']:
                if not args.should_print_all:
                    continue
                print('Fixed test:', test_testing['name'])
            else:
                print('Failed test:', test_testing['name'])
                print('APP_INFO:', test_testing['app_info'])
                print('\n'.join(dialog).encode('utf-8'))
                if turn.get('exception'):
                    print('Exception:', turn['exception'].encode('utf-8'))
                if 'setrace_reqid' in turn_stable:
                    print('SETrace id (stable): ', turn_stable['setrace_reqid'])
                if 'setrace_reqid' in turn:
                    print('SETrace id (testing):', turn['setrace_reqid'])
            print('==========================')

    return 0


if __name__ == '__main__':
    sys.exit(main())
