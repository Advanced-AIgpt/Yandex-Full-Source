# coding: utf-8
from __future__ import unicode_literals

import json
import base64
import mock
import urllib
import pytest

from vins_core.utils.strings import smart_utf8
from vins_api.webim.primitives import VinsResponse


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


def get_fake_handle_request(experiments=()):
    def fake_handle_request(req_info):

        if req_info.experiments is not None:
            for exp in experiments:
                assert req_info.experiments[exp] is not None
        else:
            assert len(experiments) == 0
        return VinsResponse()
    return fake_handle_request


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


_exps = ['disable__hello', 'disable__bye_narrow']
_v1_params = ('/webim', 'test_webim', 'webim', _message_request_v1)
_v2_params = ('/webim/v2', 'test_webim_v2', 'webim_v2', _message_request_v2)


@pytest.mark.parametrize("app_params", [_v1_params, _v2_params])
@pytest.mark.parametrize("experiments,cgi", [
    (_exps, 'experiments=disable__hello&experiments=disable__bye_narrow'),
    (_exps, 'experiments=' + urllib.quote_plus(json.dumps(_exps))),
    (_exps, 'experiments=' + urllib.quote_plus(','.join(_exps)))
])
def test_experiment_flags(webim_client, app_params, experiments, cgi):
    (url_base, app_id, resource_key, body) = app_params
    resource = webim_client[1].get(resource_key, None)
    connector = resource.get_or_create_connected_app(app_id)
    with mock.patch.object(connector, "handle_request", get_fake_handle_request(experiments)):
        resp = _webim_request(
            webim_client,
            url_base=url_base,
            body=body,
            cgi=smart_utf8(cgi),
            appname=app_id
        )
        assert resp.status_code == 200
