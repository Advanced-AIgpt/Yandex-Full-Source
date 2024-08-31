import socket
import os

import alice.uniproxy.library.utils.hostname


def test_socket_name():
    alice.uniproxy.library.utils.hostname.__hostname = None
    if 'QLOUD_DISCOVERY_INSTANCE' in os.environ:
        del os.environ['QLOUD_DISCOVERY_INSTANCE']

    assert alice.uniproxy.library.utils.hostname.current_hostname() == socket.getfqdn()
    assert alice.uniproxy.library.utils.hostname.__hostname == socket.getfqdn()


QLOUD_TEST_NAME = 'uniproxy-1.uniproxy.stable.uniproxy.voice-ext.stable.qloud-d.yandex.net'


def test_qloud_name():
    alice.uniproxy.library.utils.hostname.__hostname = None
    if 'QLOUD_DISCOVERY_INSTANCE' not in os.environ:
        os.environ['QLOUD_DISCOVERY_INSTANCE'] = QLOUD_TEST_NAME

    assert os.environ['QLOUD_DISCOVERY_INSTANCE'] == QLOUD_TEST_NAME
    assert alice.uniproxy.library.utils.hostname.current_hostname() == QLOUD_TEST_NAME
    assert alice.uniproxy.library.utils.hostname.__hostname == QLOUD_TEST_NAME


def test_replace_hostname():
    HOSTNAME = 'replace.hostname.com'
    PORT = '8888'
    url = 'https://example.com/some/path/to/file?with=params#and_fragment'
    new_url = alice.uniproxy.library.utils.hostname.replace_hostname(url, HOSTNAME, PORT)
    assert new_url == 'http://{}:{}/some/path/to/file?with=params#and_fragment'.format(HOSTNAME, PORT)
