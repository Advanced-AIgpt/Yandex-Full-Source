# coding: utf-8
from __future__ import unicode_literals

import json
import base64
import re

from copy import deepcopy

from vins_core.utils.strings import smart_utf8


_new_chat_request = {
    "event": "new_chat",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "visitor": {
        "id": "00000000-0000-0000-0000-000000000000"
    }
}

_new_chat_with_message_request = {
    "event": "new_chat",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "visitor": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "messages": [
        {
            "kind": "visitor",
            "text": "привет"
        },
        {
            "kind": "keyboard_response",
            "response": {
                "button": {"id": "hi", "text": "are;jnbgrqebijdjafnbg;kjade eabrlkae"}
            }
        },
        {
            "kind": "visitor",
            "text": "/service\nUser region: Иваново\nyuid: 2824129431556456022\npuid: 0"
        }
    ]
}

_new_message_request = {
    "event": "new_message",
    "kind": "visitor",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "text": "привет"
}

_keyboard_message_request = {
    "event": "new_message",
    "kind": "keyboard_response",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "response": {
        "button": {"id": "hi", "text": "привет"}
    }
}

_service_message_request = {
    "event": "new_message",
    "kind": "visitor",
    "chat": {
        "id": "00000000-0000-0000-0000-000000000000"
    },
    "text": "/service\nUser region: Иваново\nyuid: 2824129431556456022\npuid: 0"
}


def _webim_request(webim_client, body, header=None, appname="test_webim"):
    headers = header or {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test")
    }
    resp = webim_client[0].simulate_post(
        '/webim/%s/' % appname,
        body=smart_utf8(json.dumps(body)),
        headers=headers
    )
    return resp


def test_no_auth(webim_client):
    resp = _webim_request(webim_client, _new_chat_request, header={'content-type': b'application/json'})
    assert resp.status_code == 400


def test_bad_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:bad_password")
    }
    resp = _webim_request(webim_client, _new_chat_request, header=header)
    assert resp.status_code == 400


def test_second_user_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("another:test")
    }
    resp = _webim_request(webim_client, _new_chat_request, header=header)
    assert resp.status_code == 200


def test_bad_chat_id(webim_client):
    body = deepcopy(_new_chat_request)
    body['chat']['id'] = "not_a_valid_UUID"
    resp = _webim_request(webim_client, body)
    assert resp.status_code == 400
    body = deepcopy(_new_message_request)
    body['chat']['id'] = "not_a_valid_UUID"
    resp = _webim_request(webim_client, body)
    assert resp.status_code == 400


def test_bad_visitor_id(webim_client):
    body = deepcopy(_new_chat_request)
    body['visitor']['id'] = "not_a_valid_UUID"
    resp = _webim_request(webim_client, body)
    assert resp.status_code == 400


def test_bad_event_type(webim_client):
    resp = _webim_request(webim_client, {"event": "unknown"})
    assert resp.status_code == 400


def test_no_message(webim_client):
    req = deepcopy(_new_message_request)
    req.pop("text", None)
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 400


def test_new_chat(webim_client):
    resp = _webim_request(webim_client, _new_chat_request)
    assert resp.status_code == 200


def test_new_message(webim_client):
    resp = _webim_request(webim_client, _new_message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {
        "has_answer": True,
        "messages": [{"kind": "operator", "text": "hello, username!"}]
    }


def test_keyboard_message(webim_client):
    resp = _webim_request(webim_client, _keyboard_message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {
        "has_answer": True,
        "messages": [{"kind": "operator", "text": "hello, username!"}]
    }


def test_service_message(webim_client):
    resp = _webim_request(webim_client, _service_message_request)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {}


def test_new_chat_with_messages(webim_client):
    body = deepcopy(_new_chat_with_message_request)
    resp = _webim_request(webim_client, body)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"has_answer": False}
    body['messages'] = [body['messages'][1], body['messages'][0], body['messages'][2]]
    resp = _webim_request(webim_client, body)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {
        "has_answer": True,
        "messages": [{"kind": "operator", "text": "hello, username!"}]
    }


def test_operator_redirect(webim_client):
    req = deepcopy(_new_message_request)
    req["text"] = 'are;jnbgrqebijdjafnbg;kjade eabrlkae'
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    assert json.loads(resp.content) == {"has_answer": False}


def test_bad_message_kind(webim_client):
    req = deepcopy(_new_message_request)
    req["kind"] = "unknown"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 400


def test_response_with_button(webim_client):
    req = deepcopy(_new_message_request)
    req["text"] = "покажи мне кнопку"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    content = json.loads(resp.content)
    for i in content['messages']:
        if i['kind'] == 'keyboard':
            assert re.match(r'^[a-zA-Z0-9\\-_]{1,24}$', i['buttons'][0][0]['id'])
            i['buttons'][0][0]['id'] = '1234567'
    assert content == {
        "has_answer": True,
        "messages": [
            {"kind": "operator", "text": "hello"},
            {"kind": "keyboard", "buttons": [[{"id": '1234567', "text": "test suggest"}]]}
        ]
    }


def test_response_with_multiple_buttons(webim_client):
    req = deepcopy(_new_message_request)
    req["text"] = "покажи мне много кнопок"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    content = json.loads(resp.content)
    keyboard = next((i for i in content['messages'] if i['kind'] == 'keyboard'), None)
    assert keyboard is not None
    assert len(keyboard['buttons']) == 3
    assert len(keyboard['buttons'][0]) == 1
    assert len(keyboard['buttons'][1]) == 2
    assert len(keyboard['buttons'][2]) == 1
