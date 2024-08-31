# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import time
import six
import pytest

from subprocess import Popen, PIPE

from jupyter_client import BlockingKernelClient

import yatest.common as yc


SETUP_TIMEOUT = 30
TIMEOUT = 10


@pytest.fixture
def kernel_path():
    if six.PY2:
        binary_name = 'py2/arcadia_default_py2'
    else:
        binary_name = 'py3/arcadia_default_py3'

    return yc.binary_path(os.path.join('jupytercloud/arcadia_kernel', binary_name))


@pytest.fixture(scope='function')
def kernel(kernel_path):
    work_path = yc.work_path()
    kernel = Popen(
        [kernel_path], stdout=PIPE, stderr=PIPE,
        env={'JUPYTER_RUNTIME_DIR': work_path}

    )
    connection_file = os.path.join(
        work_path,
        'kernel-{}.json'.format(kernel.pid)
    )

    tic = time.time()
    while (
        not os.path.exists(connection_file) and
        kernel.poll() is None and
        time.time() < tic + SETUP_TIMEOUT
    ):
        time.sleep(0.1)

    if kernel.poll() is not None:
        _, e = kernel.communicate()
        raise IOError("Kernel failed to start:\n{}".format(e))

    if not os.path.exists(connection_file):
        if kernel.poll() is None:
            kernel.terminate()
            raise IOError("Connection file {} never arrived".format(connection_file))

    client = BlockingKernelClient(connection_file=connection_file)

    try:
        client.load_connection_file()
    except ValueError:
        # Файл может уже существовать, но быть еще полупустым.
        # TODO: переделать на цикл c while TIMEOUT
        time.sleep(1)
        client.load_connection_file()

    client.start_channels()
    client.wait_for_ready()

    try:
        yield client
    finally:
        client.stop_channels()
        kernel.terminate()


@pytest.fixture(scope='function')
def execute(kernel):
    def _execute(cmd):
        kernel.execute(cmd)
        if six.PY2:
            msg = kernel.get_shell_msg(block=True, timeout=TIMEOUT)
        else:
            msg = kernel.get_shell_msg(timeout=TIMEOUT)
        content = msg['content']

        assert content['status'] == 'ok'

        return content

    return _execute


@pytest.fixture(scope='function')
def inspect(kernel):
    def _inspect(value):
        kernel.inspect(value)
        if six.PY2:
            msg = kernel.get_shell_msg(block=True, timeout=TIMEOUT)
        else:
            msg = kernel.get_shell_msg(timeout=TIMEOUT)
        content = msg['content']

        assert content['status'] == 'ok'
        assert content['found']

        return content

    return _inspect


@pytest.fixture(scope='function')
def complete(kernel):
    def _complete(code, cursor_pos=None):
        kernel.complete(code, cursor_pos)

        if six.PY2:
            msg = kernel.get_shell_msg(block=True, timeout=TIMEOUT)
        else:
            msg = kernel.get_shell_msg(timeout=TIMEOUT)
        content = msg['content']

        assert content['status'] == 'ok'

        return content

    return _complete
