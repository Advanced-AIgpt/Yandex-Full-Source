import itertools
import socket
import time
import unittest.mock as mock

import pytest
from alice.library.python.utils.network import wait_port

PATCH_FUN = 'alice.library.python.utils.network.socket.socket.connect'


@pytest.mark.parametrize('exc_class', [ConnectionRefusedError, socket.timeout], ids=lambda cls: cls.__name__)
def test_wait_port_success(exc_class):
    socketMock = mock.Mock(side_effect=[exc_class(), exc_class(), exc_class(), None])
    with mock.patch(PATCH_FUN, new=socketMock):
        start_time = time.time()
        sleep_seconds = 0.1
        wait_port('ExampleYandexNet', 1029, sleep_between_attempts_seconds=sleep_seconds, timeout_seconds=sleep_seconds * 10)
        dur = time.time() - start_time
        assert dur >= sleep_seconds * 3


@pytest.mark.parametrize('exc_class', [ConnectionRefusedError, socket.timeout], ids=lambda cls: cls.__name__)
def test_wait_port_fail(exc_class):
    socketMock = mock.Mock(side_effect=itertools.repeat(exc_class()))
    with mock.patch(PATCH_FUN, new=socketMock):
        sleep_seconds = 0.1
        with pytest.raises(Exception, match='Service ExampleYandexNet seems to be not available on port 1029. Look for errors in the ExampleYandexNet log.'):
            wait_port('ExampleYandexNet', 1029, sleep_between_attempts_seconds=sleep_seconds, timeout_seconds=sleep_seconds * 2)
