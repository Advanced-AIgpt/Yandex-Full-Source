#!/usr/bin/env python2

import socket
import re
import logging

log = logging.getLogger(__name__)

_JUPYTER_HOSTNAME_RE = re.compile(
    r'^(?P<env>testing-|devel-|t)?(?:jupyter-cloud|jc)-' +
    r'(?P<username>[a-zA-Z0-9_-]+)\.[a-z]{3}.yp-c.yandex.net$'
)

def parsed_hostname():
    hostname = socket.gethostname()
    log.debug('socket.gethostname() == %s', hostname)
    match = _JUPYTER_HOSTNAME_RE.match(hostname)

    result = {}
    if match is not None:
        match_dict = match.groupdict()
        if match_dict.get('env') is not None:
            result['jupyter_cloud_environment'] = 'testing'
        else:
            result['jupyter_cloud_environment'] = 'production'

        result['jupyter_cloud_user'] = match_dict['username']

    log.debug('jupyter grain result == %s', result)
    return result
