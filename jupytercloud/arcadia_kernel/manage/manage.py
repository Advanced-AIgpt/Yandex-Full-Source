#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse

from jupytercloud.arcadia_kernel.lib.run import RunNotebookApp

from .utils import setup_logging, KERNEL_NAME
from .install import add_install_args, install
from .get_kernels import add_get_kernels_args, get_kernels


def add_shared_args(parser):
    parser.add_argument(
        '--yes', '-y', '--always-yes',
        action='store_true',
        dest='always_yes',
        help='assume Yes to all queries and do not prompt'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.set_defaults(setup_logging=True)


def parse_args():
    parser = argparse.ArgumentParser()

    subparsers = parser.add_subparsers(dest='command')

    install_command = subparsers.add_parser(
        'install',
        help=f'install {KERNEL_NAME} into environment'
    )
    add_install_args(install_command)
    add_shared_args(install_command)

    get_kernels_command = subparsers.add_parser(
        'get-kernels',
        help='get kernels archive'
    )
    add_get_kernels_args(get_kernels_command)
    add_shared_args(get_kernels_command)

    subparsers.add_parser('run-notebook')

    return parser.parse_known_args()


def main():
    args, _ = parse_args()

    if args.command == 'install':
        setup_logging(args)
        install(args)
    elif args.command == 'get-kernels':
        setup_logging(args)
        get_kernels(args)
    elif args.command == 'run-notebook':
        RunNotebookApp.launch_instance(allow_self_as_kernel=False)
    else:
        raise ValueError('unknown command')


if __name__ == '__main__':
    main()
