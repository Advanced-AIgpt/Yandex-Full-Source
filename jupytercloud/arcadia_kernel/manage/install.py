# -*- coding: utf-8 -*-

import os
import sys
import json
import shutil

from pathlib import Path

import click

from .utils import (
    temporary_directory,
    logger,
    KERNEL_NAME,
    KERNELS_TO_INSTALL
)
from .archive import extract_all_files


def add_install_args(parser):
    parser.add_argument(
        'path',
        help=f'path to a file with an {KERNEL_NAME}',
        type=Path,
    )

    destination = parser.add_mutually_exclusive_group()
    destination.add_argument(
        '--system',
        action='store_true',
        help='install kernel to the default system location (platform-specific)'
    )
    destination.add_argument(
        '--env',
        action='store_true',
        help=f'install kernel to `{sys.prefix}/share/jupyter/kernels`'
    )
    destination.add_argument(
        '--user',
        type=str,
        help='install kernel for specified user',
    )
    destination.add_argument(
        '--current-user',
        action='store_true',
        help='install kernel for current user'
    )
    destination.add_argument(
        '--custom',
        type=str,
        help='install kernel to custom directory'
    )
    parser.add_argument(
        '--dev-mode',
        action='store_true',
        help='install kernels in dev mode',
    )


def get_linux_destination(args):
    default = not (
        args.current_user or
        args.system or
        args.env or
        args.user is not None or
        args.custom is not None
    )

    if args.custom is not None:
        path = Path(args.custom)
    else:
        if args.system or default and os.geteuid() == 0:
            prefix = '/usr/local/share'
        elif args.current_user or default:
            prefix = '~/.local/share'
        elif args.user:
            prefix = f'~{args.user}/.local/share'
        elif args.env:
            prefix = os.path.join(sys.prefix, 'share')
        else:
            assert False, 'this never should happen'

        path = Path(prefix) / 'jupyter' / 'kernels'

    return path.expanduser().resolve()


def install_kernel(kernel_name, source_dir, kernels_dir, always_yes, dev_mode):
    kernel_file_rel_path = Path(kernel_name) / 'kernel.json'
    kernel_source_file_path = source_dir / kernel_file_rel_path
    kernel_source_dir_path = source_dir / kernel_name
    dest_dir = kernels_dir / kernel_name

    variables = {
        'kernel_dir': str(dest_dir),
    }

    if not kernel_source_file_path.is_file():
        raise RuntimeError('missing file {} in archive'.format(kernel_file_rel_path))

    logger.debug('loading %s', kernel_source_file_path)
    with kernel_source_file_path.open() as kernel_file:
        kernel_dict = json.load(kernel_file)

    raw_argv = kernel_dict['argv']
    if dev_mode:
        raw_argv.append('--ArcadiaKernelApp.log_level=DEBUG')
        raw_argv.append('--ArcadiaKernelApp.log_file=%(kernel_dir)s/kernel.log')

    argv = [arg % variables for arg in raw_argv]
    kernel_dict['argv'] = argv

    if 'env' in kernel_dict:
        env = kernel_dict['env']
        for key, value in env.items():
            env[key] = value % variables

    logger.debug('dumping %s', kernel_source_file_path)
    with kernel_source_file_path.open('w') as kernel_file:
        json.dump(kernel_dict, kernel_file, indent=4)

    if dest_dir.exists():
        prompt = f'Destination directory {dest_dir} for kernel {kernel_name} exists. Replace it?'

        if always_yes or click.confirm(prompt, default=True):
            logger.info('removing %s', dest_dir)
            shutil.rmtree(dest_dir)
        else:
            logger.warning('kernel %s are not installed', kernel_name)
            return

    shutil.move(kernel_source_dir_path, dest_dir)
    logger.info('kernel %s installed to %s', kernel_name, dest_dir)


def install(args):
    if not sys.platform.startswith('linux'):
        raise NotImplementedError(
            'install command currently are not adapted for non-linux platforms'
        )

    destination = get_linux_destination(args)
    logger.info('kernels would be installed to %s', destination)

    if not destination.exists():
        logger.info('creating path %s', destination)
        destination.mkdir(parents=True)

    with temporary_directory() as tmp_dir:
        extract_all_files(args.path, tmp_dir)

        for kernel_to_install in KERNELS_TO_INSTALL:
            install_kernel(
                kernel_to_install,
                tmp_dir,
                destination,
                always_yes=args.always_yes,
                dev_mode=args.dev_mode,
            )
