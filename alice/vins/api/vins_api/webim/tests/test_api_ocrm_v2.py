# coding: utf-8
from __future__ import unicode_literals

import json
import base64
import uuid
import pytz
import re
from datetime import datetime
from vins_core.common.utterance import Utterance
from vins_core.dm.request import AppInfo, ReqInfo
from vins_core.dm.request_events import TextInputEvent

from vins_api.speechkit.schemas import UUID_PATTERN
from vins_api.webim.resources.webim import WebimAPIError

from copy import deepcopy

from vins_core.utils.strings import smart_utf8

import mock

_message_request = {
    "Message": {
        "Plain": {
            "ChatId": "00000000-0000-0000-0000-000000000000",
            "Text": {
                "MessageText": "привет"
            }
        }
    }
}

_post_message_request = {
    'body': {
        "ServerMessageInfo": {
            "Timestamp": 12345678901
        },
        "Message": {
            "Plain": {
                "PayloadId": "123",
                "ChatId": "00000000-0000-0000-0000-000000000000",
                "Text": {
                    "MessageText": "привет"
                }
            }
        },
        "CustomFrom": {
            "DisplayName": "Робот Григорий"
        }
    },
    'headers': {
        'Authorization': 'Basic ' + base64.b64encode('test:test')
    },
    'url': 'http://localhost/with/a/totally/valid/ocrm/path/v2/bot'
}


_post_keyboard_request = {
    'body': {
        "ServerMessageInfo": {
            "Timestamp": 12345678901
        },
        "Message": {
            "Plain": {
                "PayloadId": "123",
                "ChatId": 1,
                "Text": {
                    "MessageText": "привет",
                    "ReplyMarkup": {
                        "inline_keyboard": [[]]
                    }
                }
            }
        },
        "CustomFrom": {
            "DisplayName": "Робот Григорий"
        }
    },
    'headers': {
        'Authorization': 'Basic ' + base64.b64encode('test:test')
    },
    'url': 'http://localhost/with/a/totally/valid/ocrm/path/v2/bot'
}


def make_text_reference(chat_id, text):
    ref = deepcopy(_post_message_request)
    ref['body']['Message']['Plain']['ChatId'] = chat_id
    ref['body']['Message']['Plain']['Text']['MessageText'] = text
    return ref


def make_keyboard_reference(chat_id, text, kbd):
    ref = deepcopy(_post_keyboard_request)
    ref['body']['Message']['Plain']['ChatId'] = chat_id
    ref['body']['Message']['Plain']['Text']['MessageText'] = text
    ref['body']['Message']['Plain']['Text']['ReplyMarkup']['inline_keyboard'] = kbd
    return ref


def make_department_redirect_reference(
    chat_id,
    dept_id='dept_default'  #
):
    return \
        {
            'body': {
                'category': dept_id
            },
            'headers': {
                'Authorization': 'Basic ' + base64.b64encode('test:test')
            },
            'url': 'http://localhost/with/a/totally/valid/ocrm/path/{}/switch/operator'.format(chat_id)
        }


class RequestForTest(object):
    def __init__(self, headers, path):
        self.headers = list((k.upper(), v.upper()) for k, v in headers.iteritems())
        self.method = 'POST'
        self.path = path


def _ocrm_request(webim_client, body, header=None, appname="test_ocrm"):
    headers = header or {
        'Content-Type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp = webim_client[0].simulate_post(
        '/ocrm/v2/%s/' % appname,
        body=smart_utf8(json.dumps(body)),
        headers=headers
    )
    return resp, RequestForTest(headers, '/ocrm/v2/%s/' % appname)


def build_req_info(chat_id, req_id, uri, text=''):
    return ReqInfo(
        uuid=chat_id,
        client_time=datetime.now(tz=pytz.timezone("Europe/Moscow")),
        app_info=AppInfo(
            app_id="test_ocrm",
            os_version="0.0.0",
            platform="unknown",
            device_manufacturer="unknown",
            device_model="unknown"
        ),
        utterance=Utterance(text=text),
        request_id=req_id,
        device_id=str(uuid.uuid3(uuid.NAMESPACE_URL, uri)),
        event=TextInputEvent(text=text)
    )


def get_session(app_resources, chat_id="1", appname=b'test_webim'):
    resource = app_resources.get('ocrm_v2', None)
    app = resource.get_or_create_connected_app("test_ocrm")
    req_info = build_req_info(
        chat_id=chat_id,
        req_id=b'00000000-0000-0000-0000-000000000000',
        uri=b'/ocrm/v2/%s/' % appname
    )
    return app.vins_app.load_or_create_session(req_info)


class RequestReference(object):
    def __init__(self, url, headers, body):
        self.url = url
        self.headers = {k.upper(): v.upper() for k, v in headers.iteritems()}
        self.body = body
        self.checked = False

    def check(self, url, headers, body):
        assert self.url == url
        self._check_body(body)
        self._check_headers(headers)
        self.checked = True

    def _check_body(self, body):
        if 'category' in body:
            assert body == self.body
        else:
            assert isinstance(body["ServerMessageInfo"]["Timestamp"], (int, long))
            assert isinstance(body["CustomFrom"]["DisplayName"], (str, unicode))
            assert isinstance(body['Message']['Plain']['PayloadId'], (str, unicode))
            assert body['Message']['Plain']['ChatId'] == self.body['Message']['Plain']['ChatId']
            assert body['Message']['Plain']['Text'] == self.body['Message']['Plain']['Text']

    def _check_headers(self, headers):
        t_headers = {k.upper(): v.upper() for k, v in headers.iteritems()}
        for k, v in self.headers.iteritems():
            assert v == t_headers[k]
        re.match('^[0-9]+/' + UUID_PATTERN[1:-1] + '(/[0-9]+)*$', t_headers[b'X-MARKET-REQ-ID'])


class ReferenceCollection(object):
    def __init__(self):
        self.collection = []

    def add_reference(self, reference):
        self.collection.append(RequestReference(reference['url'], reference['headers'], reference['body']))

    def check(self, url, headers, body):
        next(i for i in self.collection if not i.checked).check(url=url, headers=headers, body=body)

    def finished(self):
        return len(self.collection) == 0 or all(i.checked for i in self.collection)


def get_do_post(ref_collection):
    def func(url, headers, body):
        ref_collection.check(url=url, headers=headers, body=body)
    return func


def get_raising_do_post(ref_collection):
    def func(url, headers, body):
        ref_collection.check(url=url, headers=headers, body=body)
        if 'redirect' in url:
            raise WebimAPIError
    return func


def run_response_asserts(app_resources, req, collection):
    resource = app_resources.get('ocrm_v2', None)
    with mock.patch.object(resource, "_do_post", get_do_post(collection)):
        resource.respond_to_user(req)
    assert collection.finished()


def test_no_auth(webim_client):
    headers = {
        'content-type': b'application/json',
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _ocrm_request(webim_client, _message_request, header=headers)
    assert resp.status_code == 400


def test_no_requid_header(webim_client):
    headers = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test")
    }
    resp, _ = _ocrm_request(webim_client, _message_request, header=headers)
    assert resp.status_code == 500


def test_bad_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:bad_password"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _ocrm_request(webim_client, _message_request, header=header)
    assert resp.status_code == 400


def test_second_user_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("another:test"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _ocrm_request(webim_client, _message_request, header=header)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}


def test_bad_chat_id(webim_client):
    body = deepcopy(_message_request)
    body['Message']['Plain'].pop('ChatId', None)
    resp, _ = _ocrm_request(webim_client, body)
    assert resp.status_code == 400


def test_no_message(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain'].pop('Text', None)
    resp, _ = _ocrm_request(webim_client, req)
    assert resp.status_code == 400
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text'].pop('MessageText', None)
    resp, _ = _ocrm_request(webim_client, req)
    assert resp.status_code == 400


def test_session_create(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['ChatId'] = "100500-1"
    resp, _ = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    session = get_session(webim_client[1], "100500-1")
    assert session.get('first_request') == 1

    req = deepcopy(_message_request)
    req['Message']['Plain']['ChatId'] = "100500-1"
    resp, _ = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    session = get_session(webim_client[1], "100500-1")
    assert session.get('first_request') == 0


def test_new_message(webim_client):
    collection = ReferenceCollection()
    req = deepcopy(_message_request)
    req['Message']['Plain']['ChatId'] = "100500-2"
    collection.add_reference(make_text_reference("100500-2", 'hello, username!'))
    resp, req = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    run_response_asserts(webim_client[1], req, collection)


def test_operator_redirect(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = 'are;jnbgrqebijdjafnbg;kjade eabrlkae'
    req['Message']['Plain']['ChatId'] = "100500-3"
    collection = ReferenceCollection()
    collection.add_reference(make_department_redirect_reference("100500-3"))
    resp, req = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    run_response_asserts(webim_client[1], req, collection)


def test_response_with_button(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = "покажи мне кнопку"
    req['Message']['Plain']['ChatId'] = "100500-4"
    collection = ReferenceCollection()
    collection.add_reference(make_keyboard_reference("100500-4", 'hello', [[{"id": '582429810', "text": "test suggest"}]]))
    resp, req = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    run_response_asserts(webim_client[1], req, collection)


def test_response_with_multiple_buttons(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = "покажи мне много кнопок"
    req['Message']['Plain']['ChatId'] = "100500-5"
    collection = ReferenceCollection()
    collection.add_reference(
        make_keyboard_reference(
            "100500-5", 'hello',
            [
                [{"id": '-1437441209', "text": "test suggest 1"}],
                [{"id": '861614845', "text": "test suggest 2"}, {"id": '1146880619', "text": "test suggest 3"}],
                [{"id": '-633825336', "text": "test suggest 4"}]
            ]
        )
    )
    resp, req = _ocrm_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}
    run_response_asserts(webim_client[1], req, collection)
