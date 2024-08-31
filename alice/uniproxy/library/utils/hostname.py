import socket
import os
import urllib


__hostname = None


def current_hostname():
    global __hostname

    if __hostname is None:
        qloud_discovery_instance = os.environ.get('QLOUD_DISCOVERY_INSTANCE')
        if qloud_discovery_instance:
            __hostname = qloud_discovery_instance
        else:
            __hostname = socket.getfqdn()

    return __hostname


def replace_hostname(url, hostname, port, scheme='http'):
    u = urllib.parse.urlsplit(url)
    return u._replace(scheme=scheme, netloc='{}:{}'.format(hostname, port)).geturl()
