# -*- coding: utf-8 -*-

import sys
import math
import logging
import time
import tempfile
import socket
import subprocess
import requests

from urllib3.util.retry import Retry
from requests.adapters import HTTPAdapter
from requests.exceptions import ChunkedEncodingError, ConnectionError
from pathlib import Path
from contextlib import contextmanager
from tqdm import tqdm


KERNELS_TO_INSTALL = ('arcadia_default_py2', 'arcadia_default_py3')
KERNEL_NAME = 'Arcadia Python kernel'

BLOCK_SIZE = 1024 ** 2
BLOCK_NAME = 'MB'

logger = logging.getLogger(__name__)


def setup_logging(args):
    level = max(1, logging.WARNING - args.verbose * 10)
    log_format = (
        "[%(levelname)1.1s %(asctime)s.%(msecs).03d %(name)s %(module)s:%(lineno)d]"
        " %(message)s"
    )

    formatter = logging.Formatter(log_format)
    handler = logging.StreamHandler(sys.stderr)
    handler.setFormatter(formatter)

    logger.setLevel(level)
    logger.addHandler(handler)


def get_steps(size):
    return math.ceil(size / BLOCK_SIZE)


@contextmanager
def temporary_directory():
    with tempfile.TemporaryDirectory(suffix='-arcadia-jupyter-kernel') as tmp_dir:
        yield Path(tmp_dir)


@contextmanager
def measure_time(text):
    start = time.monotonic()

    try:
        yield
    finally:
        result = time.monotonic() - start
        logger.info('%s took %0.2f seconds', text, result)


def check_bin_exists(name):
    result = subprocess.call(
        ['which', name],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    return result == 0


def download_file(fileobj, url):
    SLEEP = 5
    TRIES = 5

    session = requests.Session()
    retry_policy = Retry(
        total=None,
        connect=TRIES * 2,
        read=TRIES,
        status=TRIES,
        status_forcelist=[],
        backoff_factor=2.0
    )
    http_adapter = HTTPAdapter(max_retries=retry_policy)
    session.mount(url, http_adapter)

    for i in range(1, TRIES + 1):
        logger.info('downloading file from %s, try number %d', url, i)
        try:
            response = session.get(url, stream=True)
            total_size = int(response.headers.get('content-length', 0))
            read_size = 0

            fileobj.seek(0)

            for data in tqdm(
                response.iter_content(BLOCK_SIZE),
                total=get_steps(total_size),
                unit=BLOCK_NAME,
            ):
                read_size += len(data)
                fileobj.write(data)

            break
        except (
            ChunkedEncodingError,
            ConnectionError,
            socket.timeout
        ) as e:
            logger.error(
                'got retrieable error while downloading %s, sleep %d',
                url, SLEEP, exc_info=e
            )
            time.sleep(SLEEP)
    else:
        raise RuntimeError('all download tries failed')

    if read_size != total_size:
        raise ValueError(
            f'archive downloaded file size {read_size} do not equal expected {total_size}'
        )

    fileobj.seek(0)

    return read_size


def sky_download_file(skynet_id, dest_dir):
    cmd = ['sky', 'get', '-p', '--progress-format=json', skynet_id]
    logger.info('downloading file via %r', cmd)

    def log_subprocess_output(pipe):
        for line in iter(pipe.readline, b''):
            logger.debug('%r output: %r', cmd, line)

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=dest_dir,
    )

    while process.poll() is None:
        log_subprocess_output(process.stdout)

    exitcode = process.wait()

    if exitcode:
        raise RuntimeError(f'{cmd} exited with non-zero exit code, {exitcode}')
