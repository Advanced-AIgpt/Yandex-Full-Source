# coding: utf-8
from __future__ import unicode_literals

import json
import base64
import pytest
import yatest.common
import re

import logging

from vins_core.utils.strings import smart_utf8

_message_request_v1 = {
    "event": "new_message",
    "kind": "visitor",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "text": "привет"
}

_message_request_v2 = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "visitor",
        "text": "привет",
        "id": "00000000-0000-0000-0000-000000000000"
    }
}


def _webim_request(webim_client, url_base, body, header=None, appname="test_webim_v2", cgi=None):
    headers = header or {
        'Content-Type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    url = url_base + '/' + appname + '/'
    resp = webim_client[0].simulate_post(
        url,
        body=smart_utf8(json.dumps(body)),
        headers=headers,
        query_string=cgi
    )
    return resp


class TraceLogSetup(object):
    def __init__(self, path):
        self.path = path
        self.handler = logging.FileHandler(self.path)
        self.handler.setFormatter(logging.Formatter(b'%(message)s'))

    def __enter__(self):
        logger = logging.getLogger('TSUM_trace')
        logger.addHandler(self.handler)

    def __exit__(self, exc_type, exc_val, exc_tb):
        logger = logging.getLogger('TSUM_trace')
        logger.removeHandler(self.handler)
        return False


_v1_params = ('/webim', 'test_webim', _message_request_v1)
_v2_params = ('/webim/v2', 'test_webim_v2', _message_request_v2)

trace_reference = {
    'query_params': r'^/webim(/v2)?',
    'protocol': r'^http$',
    'retry_num': r'^[\d]*$',
    'date': r'^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]{3,6}\+[0-9]{4}$',
    'http_method': r'^POST$',
    'source_host': r'^127\.0\.0\.1$',
    'duration_ms': r'^[\d]+$',
    'response_size_bytes': r'^[\d]+$',
    'type': r'^IN$',
    'source_module': r'^[a-zA-Z_0-9]*$',
    'request_id': r'^[0-9]{13}/[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}$',
    'request_method': r'^[/a-zA-Z0-9_]+$',
    'error_code': r'^[a-zA-Z0-9_ ]*$',
    'http_code': r'^[\d]{3}$',
    'target_host': r'^falconframework\.org',
    'target_module': r'^lilucrmchat$'
}


@pytest.mark.parametrize("app_params", [_v1_params, _v2_params])
def test_logging(webim_client, app_params):
    (url_base, app_id, body) = app_params
    with TraceLogSetup(yatest.common.test_output_path('tsum_trace.log')):
        resp = _webim_request(
            webim_client,
            url_base=url_base,
            body=body,
            appname=app_id
        )
        assert resp.status_code == 200
        with open(yatest.common.test_output_path('tsum_trace.log'), 'r') as f:
            lines = [l for l in f.read().splitlines() if l != '']
            assert len(lines) == 1
            line = lines[0]
            fields = line.split('\t')
            assert fields[0] == 'tskv'
            assert fields[1] == 'tskv_format=trace-log'
            for field in fields[2:]:
                (key, value) = field.split('=', 1)
                value = value.replace('/=', '=')
                assert re.match(trace_reference[key], value) is not None, "Problem with the \"{}\" field".format(key)
