#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import argparse
import contextlib
import logging
import os
import stat
import shutil
import time
import tempfile

from subprocess import Popen, PIPE, STDOUT

import requests
from requests.adapters import HTTPAdapter

BASE_URL = 'https://sandbox.yandex-team.ru/api/v1.0'
RESOURCE_NAME = 'JUPYTER_ARCADIA_KERNELS_MANAGE'

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s [%(levelname)s] %(message)s')


ARTEFACTS = {
    'manage': 'manager',
    'arcadia_default_py2': 'kernel_py2',
    'arcadia_default_py3': 'kernel_py3',
    'arcadia_default_py2_yql.so': 'kernel_py2_yql.so',
    'arcadia_default_py3_yql.so': 'kernel_py3_yql.so',
}


def parse_args():
    parser = argparse.ArgumentParser()

    for default, artefact in ARTEFACTS.iteritems():
        artefact = artefact.replace('_', '-').replace('.', '-')
        parser.add_argument('--dst-{}'.format(artefact), default=default)

    parser.add_argument('--kernel-py3', type=str)
    parser.add_argument('--sky-get-timeout', default=30, type=int)

    parser.add_argument('--kernel-revision', type=str, default='')
    parser.add_argument('--kernel-dir', type=str, default=None)
    parser.add_argument('--get-nile-over-yql-binaries', action='store_true')

    return parser.parse_args()


def request(method, url, **kwargs):
    s = requests.Session()
    s.mount(url, HTTPAdapter(max_retries=5))

    response = s.request(method, url, **kwargs)
    response.raise_for_status()

    return response


def get_last_resource_info():
    uri = '/resource?type={}&limit=1&order=-id&state=READY'.format(RESOURCE_NAME)
    url = BASE_URL + uri

    response = request('GET', url)
    result = response.json()
    items = result.get('items', [])

    if not items:
        raise RuntimeError(
            'failed to fetch meta for last {} resource'
            .format(RESOURCE_NAME)
        )

    return items[0]


def download_file(url, file_name):
    logging.info('download file %s to %s', url, file_name)

    response = request('GET', url, stream=True)

    with open(file_name, 'wb') as file_:
        for data in response.iter_content(1024 ** 2):
            file_.write(data)


def move(src, dst):
    logging.debug('moving %s to %s', src, dst)
    shutil.move(src, dst)


def check_call(cmd, stdout=None, cwd=None):
    logging.debug('running command %r', cmd)

    def log_subprocess_output(pipe):
        for line in iter(pipe.readline, b''):
            logging.debug('%r output: %r', cmd, line)

    stdout = stdout or PIPE
    stderr = STDOUT if stdout == PIPE else PIPE

    process = Popen(cmd, stdout=stdout, stderr=stderr, cwd=cwd)

    out = process.stdout if stdout == PIPE else process.stderr
    with out:
        log_subprocess_output(out)

    exitcode = process.wait()

    if exitcode:
        raise RuntimeError('{} exited with non-zero exit code'.format(exitcode))


def log_system_info():
    check_call(['df', '-h', '.'])
    check_call(['free', '-t', '-h'])


@contextlib.contextmanager
def log_time(title):
    start = time.time()

    yield

    logging.info('%s took %0.2f seconds', title, time.time() - start)


def log_filesize(file_name):
    st = os.stat(file_name)
    size = st.st_size / 1024 ** 2
    logging.info('%s size %0.2f MB', file_name, size)


def download_manager(dst_manager, sky_get_timeout):
    with log_time('fetching info about last resource'):
        last_resource = get_last_resource_info()

    skynet_id = last_resource['skynet_id']
    file_name = last_resource['file_name']
    if file_name.startswith('pack/'):
        file_name = file_name[len('pack/'):]

    dirname = os.path.dirname(dst_manager) or '.'
    dst = os.path.join(dirname, file_name)

    if not os.path.exists(dirname):
        os.makedirs(dirname)

    sky_success = True
    with log_time('downloading resource via sky get'):
        timeout = str(sky_get_timeout)
        try:
            check_call(
                ['sky', 'get', '-w', '-t', timeout, '-p', '--progress-format=json', skynet_id],
                cwd=dirname
            )
        except RuntimeError:
            sky_success = False
            logging.error('sky get exited with non-zero status', exc_info=True)

    if not sky_success:
        url = last_resource['http']['proxy']
        with log_time('fallback on downloading resource via requests'):
            download_file(url, file_name)

    st = os.stat(dst)
    os.chmod(dst, st.st_mode | stat.S_IEXEC)

    if dst != dst_manager:
        move(dst, dst_manager)



def download_kernels(manager, kernel_revision, dirname):
    kernel_revision = kernel_revision.strip()

    command = [manager, 'get-kernels', '-vvv', '--dst-dir', dirname]
    if kernel_revision:
        command += ['--svn-revision', kernel_revision]
    else:
        command += ['--last']

    with log_time('downloading kernels'):
        check_call(command)


def main():
    args = parse_args()

    log_system_info()

    download_manager(
        dst_manager=args.dst_manager,
        sky_get_timeout=args.sky_get_timeout
    )

    log_filesize(args.dst_manager)

    if args.kernel_py3:
        move(args.kernel_py3, args.dst_kernel_py3)
        return

    kernel_dir = args.kernel_dir or tempfile.mkdtemp()

    try:
        download_kernels(
            manager=os.path.abspath(args.dst_manager),
            kernel_revision=args.kernel_revision,
            dirname=kernel_dir
        )

        for filename, artefact in ARTEFACTS.iteritems():
            option = artefact.replace('-', '_').replace('.', '_')
            option_value = getattr(args, 'dst_{}'.format(option))

            if (
                filename == 'manage' or
                filename.endswith('.so') and not args.get_nile_over_yql_binaries
            ):
                continue

            path = os.path.join(kernel_dir, filename)

            move(path, option_value)

    finally:
        if not args.kernel_dir:
            # it is tmp dir
            shutil.rmtree(kernel_dir)


if __name__ == '__main__':
    main()
