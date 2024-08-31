# coding: utf-8
from __future__ import unicode_literals

import json
import base64
import struct
import uuid
import pytz
from datetime import datetime
from vins_core.common.utterance import Utterance
from vins_core.dm.request import AppInfo, ReqInfo
from vins_core.dm.request_events import TextInputEvent

from Crypto.Cipher import AES
from vins_api.webim.crypto import Decryptor
from vins_api.webim.resources.webim import WebimAPIError

from copy import deepcopy

from vins_core.utils.strings import smart_utf8

import mock

_new_chat_request = {
    "event": "new_chat",
    "chat": {
        "id": 1
    },
    "visitor": {
        "id": "00000000-0000-0000-0000-000000000000",
        "fields": {
            "name": "John",
            "phone": "+71234567890"
        }
    }
}

_message_request = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "visitor",
        "text": "привет",
        "id": "00000000-0000-0000-0000-000000000000"
    }
}

_new_chat_with_message_request = {
    "event": "new_chat",
    "chat": {
        "id": 1
    },
    "visitor": {
        "id": "00000000-0000-0000-0000-000000000000",
        "fields": {
            "name": "John",
            "phone": "+71234567890"
        }
    },
    "messages": [
        {
            "id": "00000000-0000-0000-0000-000000000001",
            "kind": "visitor",
            "text": "привет"
        },
        {
            "kind": "keyboard_response",
            "data": {
                "button": {"id": "hi", "text": "are;jnbgrqebijdjafnbg;kjade eabrlkae"},
                "request": {"messageId": "00000000-0000-0000-0000-000000000000"}
            },
            "id": "00000000-0000-0000-0000-000000000002"
        },
        {
            "id": "00000000-0000-0000-0000-000000000003",
            "kind": "visitor",
            "text": "/service\nUser region: Иваново\nyuid: 2824129431556456022\npuid: 0"
        }
    ]
}

_post_message_request = {
    'body': {
        'chat_id': 1,
        'message': {
            "kind": "operator",
            "text": "blah-blah"
        }
    },
    'headers': {
        'Authorization': 'Token super_auth_token'
    },
    'url': 'http://localhost/with/a/totally/valid/path/send_message'
}


_file_request = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "file_visitor",
        "data": {
            "id": "00000000-0000-0000-0000-000000000000",
            "name": "cool_image.png",
            "media_type": "image/png",
            "size": 123
        },
        "id": "00000000-0000-0000-0000-000000000000"
    }
}

_post_file_request = {
    'body': {
        'chat_id': 1,
        'message': {
            "kind": "file_operator",
            "data": {
                "url": "http://a_valid.url/path",
                "name": "a_valid.name",
                "media_type": "something/something"
            }
        }
    },
    'headers': {
        'Authorization': 'Token super_auth_token'
    },
    'url': 'http://localhost/with/a/totally/valid/path/send_message'
}

_button_request = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "keyboard_response",
        "data": {
            "button": {"id": "hi", "text": "привет"},
            "request": {"messageId": "00000000-0000-0000-0000-000000000000"}
        },
        "id": "00000000-0000-0000-0000-000000000000"
    }
}

_service_message_request = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "visitor",
        "text": "/service\nUser region: Иваново\nyuid: 2824129431556456022\npuid: 0",
        "id": "00000000-0000-0000-0000-000000000000"
    }
}

_file_message_request = {
    "event": "new_message",
    "chat_id": 1,
    "message": {
        "kind": "file_visitor",
        "data": {
            "url": "https://testberu.webim.ru/api/bot/v2/file/c068ffe3a0774095825cb41937458329?"
                   "hash=a0196f6b59ec2197eaaa2cf0f933faa365a7be98986d191ab148828eaf8ee16b",
            "media_type": "text/plain",
            "name": "t.txt",
            "size": 24
        },
        "id": "f1fee54fd0b34dbf8c8709922e388b14"
    }
}


def make_text_reference(chat_id, text):
    return \
        {
            'body': {
                'chat_id': chat_id,
                'message': {
                    "kind": "operator",
                    "text": text
                }
            },
            'headers': {
                'Authorization': 'Token super_auth_token'
            },
            'url': 'http://localhost/with/a/totally/valid/path/send_message'
        }


def make_keyboard_reference(chat_id, kbd):
    return \
        {
            'body': {
                'chat_id': chat_id,
                'message': {
                    "kind": "keyboard",
                    "buttons": kbd
                }
            },
            'headers': {
                'Authorization': 'Token super_auth_token'
            },
            'url': 'http://localhost/with/a/totally/valid/path/send_message'
        }


def make_department_redirect_reference(
    chat_id,
    dept_id='dept_default'  # , ignoring the parameters below for now
    # to_offline=False,
    # to_inv=True
):
    return \
        {
            'body': {
                'chat_id': chat_id,
                'dep_key': dept_id,
                # "allow_redirect_to_offline_dep": to_offline,
                # "allow_redirect_to_invisible_dep": to_inv
            },
            'headers': {
                'Authorization': 'Token super_auth_token'
            },
            'url': 'http://localhost/with/a/totally/valid/path/redirect_chat'
        }


def make_operator_redirect_reference(chat_id, operator_id):
    return \
        {
            'body': {
                'chat_id': chat_id,
                'operator_id': operator_id
            },
            'headers': {
                'Authorization': 'Token super_auth_token'
            },
            'url': 'http://localhost/with/a/totally/valid/path/redirect_chat'
        }


def make_close_chat_reference(chat_id):
    return \
        {
            'body': {
                'chat_id': chat_id
            },
            'headers': {
                'Authorization': 'Token super_auth_token'
            },
            'url': 'http://localhost/with/a/totally/valid/path/close_chat'
        }


def _pad(msg):
    return msg + (AES.block_size - len(msg) % AES.block_size) * chr(AES.block_size - len(msg) % AES.block_size)


def encrypt(msg, key_version):
    raw = _pad(msg)
    iv = b"tehsuperduperrandomiv"[:16]
    key = Decryptor().keys['key_v{}'.format(key_version)]
    cipher = AES.new(key, AES.MODE_CBC, iv)
    return base64.b64encode(struct.pack('>i', key_version) + iv + cipher.encrypt(raw))


class RequestForTest(object):
    def __init__(self, headers, path):
        self.headers = list((k.upper(), v.upper()) for k, v in headers.iteritems())
        self.method = 'POST'
        self.path = path


def _webim_v2_request(webim_client, body, header=None, appname="test_webim_v2"):
    headers = header or {
        'Content-Type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp = webim_client[0].simulate_post(
        '/webim/v2/%s/' % appname,
        body=smart_utf8(json.dumps(body)),
        headers=headers
    )
    return resp, RequestForTest(headers, '/webim/v2/%s/' % appname)


def build_req_info(chat_id, req_id, uri, text=''):
    return ReqInfo(
        uuid=uuid.UUID(int=chat_id),
        client_time=datetime.now(tz=pytz.timezone("Europe/Moscow")),
        app_info=AppInfo(
            app_id="test_webim_v2",
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


def get_session(app_resources, chat_id=1, appname=b'test_webim'):
    resource = app_resources.get('webim', None)
    app = resource.get_or_create_connected_app("test_webim_v2")
    req_info = build_req_info(
        chat_id=chat_id,
        req_id=b'00000000-0000-0000-0000-000000000000',
        uri=b'/webim/v2/%s/' % appname
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
        assert self.body == body
        t_headers = {k.upper(): v.upper() for k, v in headers.iteritems()}
        for k, v in self.headers.iteritems():
            assert v == t_headers[k]
        self.checked = True


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
    resource = app_resources.get('webim_v2', None)
    with mock.patch.object(resource, "_do_post", get_do_post(collection)):
        resource.respond_to_user(req)
    assert collection.finished()


def test_no_auth(webim_client):
    headers = {
        'content-type': b'application/json',
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _webim_v2_request(webim_client, _new_chat_request, header=headers)
    assert resp.status_code == 400


def test_no_requid_header(webim_client):
    headers = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test")
    }
    resp, _ = _webim_v2_request(webim_client, _new_chat_request, header=headers)
    assert resp.status_code == 500


def test_bad_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:bad_password"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _webim_v2_request(webim_client, _new_chat_request, header=header)
    assert resp.status_code == 400


def test_second_user_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("another:test"),
        'X-Market-Req-ID': b'0000000000000/00000000-0000-0000-0000-000000000000'
    }
    resp, _ = _webim_v2_request(webim_client, _new_chat_request, header=header)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}


def test_bad_chat_id(webim_client):
    body = deepcopy(_new_chat_request)
    body['chat']['id'] = "not_a_valid_integer"
    resp, _ = _webim_v2_request(webim_client, body)
    assert resp.status_code == 400
    body = deepcopy(_message_request)
    body['chat_id'] = "not_a_valid_integer"
    resp, _ = _webim_v2_request(webim_client, body)
    assert resp.status_code == 400


def test_bad_visitor_id(webim_client):
    body = deepcopy(_new_chat_request)
    body['visitor']['id'] = "not_a_valid_UUID"
    resp, _ = _webim_v2_request(webim_client, body)
    assert resp.status_code == 400


def test_bad_event_type(webim_client):
    resp, _ = _webim_v2_request(webim_client, {"event": "unknown"})
    assert resp.status_code == 400


def test_no_message(webim_client):
    req = deepcopy(_message_request)
    req["message"].pop("text", None)
    resp, _ = _webim_v2_request(webim_client, req)
    assert resp.status_code == 400


def test_new_chat(webim_client):
    resp, _ = _webim_v2_request(webim_client, _new_chat_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}


def test_session_create(webim_client):
    resp, _ = _webim_v2_request(webim_client, _new_chat_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    session = get_session(webim_client[1])
    assert session.get('visitor_id') == _new_chat_request["visitor"]["id"]
    assert session.get('visitor_fields') == _new_chat_request["visitor"]["fields"]
    assert session.get('first_request') == 1


def test_secure_data(webim_client):
    new_chat_with_sec_data = deepcopy(_new_chat_request)
    sec_data = {b"very_important_data": b"it_works"}
    new_chat_with_sec_data["chat"]["id"] = 2
    new_chat_with_sec_data["visitor"]["fields"]["sec_data"] = encrypt(json.dumps(sec_data), 1)
    resp, _ = _webim_v2_request(webim_client, new_chat_with_sec_data)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    del new_chat_with_sec_data["visitor"]["fields"]["sec_data"]
    session = get_session(webim_client[1], chat_id=new_chat_with_sec_data["chat"]["id"])
    assert session.get('visitor_id') == new_chat_with_sec_data["visitor"]["id"]
    assert session.get('visitor_fields') == new_chat_with_sec_data["visitor"]["fields"]
    assert session.get('sec_data') == sec_data


def test_new_message(webim_client):
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(1, 'hello, username!'))
    resp, req = _webim_v2_request(webim_client, _message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_file_message(webim_client):
    resp, req = _webim_v2_request(webim_client, _file_message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}


def test_new_chat_with_messages_1(webim_client):
    body = deepcopy(_new_chat_with_message_request)
    collection = ReferenceCollection()
    collection.add_reference(make_department_redirect_reference(1))
    resp, req = _webim_v2_request(webim_client, body)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_new_chat_with_messages_2(webim_client):
    body = deepcopy(_new_chat_with_message_request)
    body['messages'] = [body['messages'][1], body['messages'][0], body['messages'][2]]
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(1, 'hello, username!'))
    resp, req = _webim_v2_request(webim_client, body)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_keyboard_message(webim_client):
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(1, 'hello, username!'))
    resp, req = _webim_v2_request(webim_client, _button_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_service_message(webim_client):
    collection = ReferenceCollection()
    resp, req = _webim_v2_request(webim_client, _service_message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_operator_redirect(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = 'are;jnbgrqebijdjafnbg;kjade eabrlkae'
    req["chat_id"] = 202
    collection = ReferenceCollection()
    collection.add_reference(make_department_redirect_reference(202))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_bad_message_kind(webim_client):
    req = deepcopy(_message_request)
    req["message"]["kind"] = "unknown"
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 400
    run_response_asserts(webim_client[1], req, ReferenceCollection())


def test_response_with_button(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = "покажи мне кнопку"
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(1, 'hello'))
    collection.add_reference(make_keyboard_reference(1, [[{"id": '582429810', "text": "test suggest"}]]))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_response_with_multiple_buttons(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = "покажи мне много кнопок"
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(1, 'hello'))
    collection.add_reference(
        make_keyboard_reference(
            1,
            [
                [{"id": '-1437441209', "text": "test suggest 1"}],
                [{"id": '861614845', "text": "test suggest 2"}, {"id": '1146880619', "text": "test suggest 3"}],
                [{"id": '-633825336', "text": "test suggest 4"}]
            ]
        )
    )
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_targeted_operator_redirect(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = 'переведи меня на особого оператора'
    req["chat_id"] = 202
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(chat_id=202, text='Redirecting you to a very special operator'))
    collection.add_reference(make_operator_redirect_reference(chat_id=202, operator_id=123456789))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_targeted_department_redirect(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = 'переведи меня в блатной отдел'
    req["chat_id"] = 202
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(chat_id=202, text='Redirecting you to a very special department'))
    collection.add_reference(make_department_redirect_reference(chat_id=202, dept_id='123456789.qwer-ty'))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_close_chat(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = 'закрой этот чат'
    req["chat_id"] = 205
    collection = ReferenceCollection()
    collection.add_reference(make_text_reference(chat_id=205, text='Closing this chat.'))
    collection.add_reference(make_close_chat_reference(chat_id=205))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    run_response_asserts(webim_client[1], req, collection)


def test_failed_operator_redirect(webim_client):
    req = deepcopy(_message_request)
    req["message"]["text"] = 'are;jnbgrqebijdjafnbg;kjade eabrlkae'
    req["chat_id"] = 202
    collection = ReferenceCollection()
    collection.add_reference(make_department_redirect_reference(202))
    resp, req = _webim_v2_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"result": "ok"}
    resource = webim_client[1].get('webim_v2', None)
    collection.add_reference(make_text_reference(202, "Everything went terribly wrong"))
    with mock.patch.object(resource, "_do_post", get_raising_do_post(collection)):
        resource.respond_to_user(req)
    assert collection.finished()
