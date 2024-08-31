# coding: utf-8
from __future__ import unicode_literals

import json
import base64

from copy import deepcopy

from vins_core.utils.strings import smart_utf8


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


def _check_response(response, chat_id, texts=[], kbd=None, redirect=False):
    assert response['switch_operator'] == redirect
    assert len(response['messages']) == len(texts)
    for msg, ref in zip(response['messages'], texts):
        assert "ServerMessageInfo" in msg
        assert "CustomFrom" in msg
        assert "Message" in msg
        assert isinstance(msg["Message"]["Plain"]["PayloadId"], (str, unicode))
        assert msg["Message"]["Plain"]["ChatId"] == chat_id
        assert msg["Message"]["Plain"]["Text"]["MessageText"] == ref
    if kbd is not None:
        msg = response['messages'][-1]
        keyboard = msg["Message"]["Plain"]["Text"]["ReplyMarkup"]["inline_keyboard"]
        assert len(keyboard) == len(kbd)
        for a_row, ref_row in zip(keyboard, kbd):
            assert len(a_row) == len(ref_row)
            for a, r in zip(a_row, ref_row):
                assert a['text'] == r['text']


def _webim_request(webim_client, body, header=None, appname="test_webim"):
    headers = header or {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:test")
    }
    resp = webim_client[0].simulate_post(
        '/ocrm/%s/' % appname,
        body=smart_utf8(json.dumps(body)),
        headers=headers
    )
    return resp


def test_no_auth(webim_client):
    resp = _webim_request(webim_client, _message_request, header={'content-type': b'application/json'})
    assert resp.status_code == 400


def test_bad_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("test:bad_password")
    }
    resp = _webim_request(webim_client, _message_request, header=header)
    assert resp.status_code == 400


def test_second_user_auth(webim_client):
    header = {
        'content-type': b'application/json',
        'Authorization': b'Basic %s' % base64.b64encode("another:test")
    }
    resp = _webim_request(webim_client, _message_request, header=header)
    assert resp.status_code == 200


def test_no_message(webim_client):
    req = deepcopy(_message_request)
    req["Message"]["Plain"]["Text"].pop("MessageText", None)
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 400


def test_no_chat_id(webim_client):
    req = deepcopy(_message_request)
    req["Message"]["Plain"].pop("ChatId", None)
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 400


def test_new_message(webim_client):
    resp = _webim_request(webim_client, _message_request)
    assert resp.status_code == 200
    response = json.loads(resp.content)
    _check_response(response, chat_id="00000000-0000-0000-0000-000000000000",
                    texts=["hello, username!"], kbd=None, redirect=False)


def test_operator_redirect(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = 'are;jnbgrqebijdjafnbg;kjade eabrlkae'
    req['Message']['Plain']['ChatId'] = "100500-3"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    response = json.loads(resp.content)
    _check_response(response, chat_id="100500-3",
                    texts=[], kbd=None, redirect=True)


def test_response_with_button(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = "покажи мне кнопку"
    req['Message']['Plain']['ChatId'] = "100500-4"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    response = json.loads(resp.content)
    _check_response(response, chat_id="100500-4",
                    texts=["hello"], kbd=[[{"text": "test suggest"}]], redirect=False)


def test_response_with_multiple_buttons(webim_client):
    req = deepcopy(_message_request)
    req['Message']['Plain']['Text']['MessageText'] = "покажи мне много кнопок"
    req['Message']['Plain']['ChatId'] = "100500-5"
    resp = _webim_request(webim_client, req)
    assert resp.status_code == 200
    response = json.loads(resp.content)
    ref_kbd = [
        [{"text": "test suggest 1"}],
        [{"text": "test suggest 2"}, {"text": "test suggest 3"}],
        [{"text": "test suggest 4"}]
    ]
    _check_response(response, chat_id="100500-5",
                    texts=["hello"], kbd=ref_kbd, redirect=False)
