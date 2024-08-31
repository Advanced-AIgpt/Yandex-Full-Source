# coding: utf-8

from __future__ import unicode_literals

import json
import pytest
import requests_mock

from requests import HTTPError
from vins_core.ext.speechkit_api import SpeechKitHTTPAPI
from vins_core.dm.request import AppInfo, ReqInfo
from vins_core.dm.request_events import SuggestedInputEvent
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.utils.datetime import utcnow


@pytest.fixture(scope='function')
def sk_api():
    return SpeechKitHTTPAPI()


@pytest.fixture(scope='module')
def app_info():
    return AppInfo(
        app_id='com.yandex.vins.tests',
        app_version='0.0.1',
        os_version='1',
        platform='unknown'
    )


@pytest.fixture(scope='module')
def req_info(app_info):
    return ReqInfo(
        request_id=str(gen_uuid_for_tests()),
        client_time=utcnow(),
        event=SuggestedInputEvent("Погода на два дня"),
        uuid=str(gen_uuid_for_tests()),
        app_info=app_info,
    )


@pytest.mark.parametrize("voice_session,reset_session", [
    (True, True),
    (True, False),
    (False, True),
    (False, False)
])
def test_forwarding_voice_and_reset_session_flags(sk_api, app_info, voice_session, reset_session):
    req_info = ReqInfo(
        request_id=str(gen_uuid_for_tests()),
        client_time=utcnow(),
        event=SuggestedInputEvent("Погода на два дня"),
        uuid=str(gen_uuid_for_tests()),
        app_info=app_info,
        reset_session=reset_session,
        voice_session=voice_session
    )
    request = sk_api.make_request(req_info)
    assert request['request']['voice_session'] == voice_session
    assert request['request']['reset_session'] == reset_session


def test_no_voice_session_if_not_set(sk_api, req_info):
    request = sk_api.make_request(req_info)
    assert 'voice_session' not in request['request']


def test_sk_http_api_raises_on_500_code(sk_api, req_info):
    with requests_mock.Mocker() as m, pytest.raises(HTTPError):
        m.post(SpeechKitHTTPAPI.PA_DEFAULT_HOST, status_code=500)
        sk_api._call_sk_api(req_info)


def test_sk_http_api_doesnt_raise_when_got_512_code(sk_api, req_info):
    resp = {
        'voice_response': None,
        'response': {
            'directives': [
                {
                    'type': 'client_action',
                    'name': 'some_name',
                    'payload': {
                        'some': 'payload'
                    }
                }
            ],
            'cards': []

        }
    }

    with requests_mock.Mocker() as m:
        m.post(SpeechKitHTTPAPI.PA_DEFAULT_HOST,
               status_code=512,
               text=json.dumps(resp),
               headers={'X-Yandex-Vins-OK': 'true'})
        assert sk_api._call_sk_api(req_info) == resp
