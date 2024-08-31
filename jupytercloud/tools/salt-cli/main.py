# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import sys

import pprint
from jupytercloud.tools.lib import environment, utils
from jupytercloud.backend.lib.clients import salt
from jupytercloud.tools.lib.report import global_report


logger = None


def parse_args():
    parser = argparse.ArgumentParser()
    environment.add_cli_arguments(parser)

    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.add_argument('--threads', '-j', type=int, default=None)

    subparsers = parser.add_subparsers(dest='command')

    restart_minions = subparsers.add_parser('restart-minions')
    restart_minions.add_argument('--users', nargs='*')

    state_apply = subparsers.add_parser('state-apply')
    state_apply.add_argument('--users', nargs='*')
    state_apply.add_argument('--no-restart', action='store_true')
    state_apply.add_argument('--timeout', type=int, default=3 * 60 * 60)
    state_apply.add_argument('state', default=None, nargs='?')

    return parser.parse_args()


def restart_minions(args):
    salt_client = salt.Salt()
    salt_client.restart_minions(users=args.users)


def state_apply(args):
    salt_client = salt.Salt()

    if not args.no_restart:
        salt_client.restart_minions(users=args.users)

    good_minions = salt_client.get_alive_minions(users=args.users)

    logger.info('going to apply state %r to %d minions',
                (args.state or 'highstate'), len(good_minions))

    logger.debug('minions to apply: %s', good_minions)

    salt_client.apply_state(good_minions, state=args.state, timeout=args.timeout)


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    with environment.from_cli_arguments(args), global_report() as report:
        {
            'restart-minions': restart_minions,
            'state-apply': state_apply
        }[args.command](args)

        print('Statistics:')
        pprint.pprint(report.aggregated())

        if report.have_errors:
            print()
            print('Errors:')
            print(report.error_report())
            sys.exit(1)
