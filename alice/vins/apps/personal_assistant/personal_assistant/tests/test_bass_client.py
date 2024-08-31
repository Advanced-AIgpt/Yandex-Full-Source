# coding: utf-8

import errno
import json
import socket
import struct
import time
import threading
import requests

import pytest

from vins_core.dm.form_filler.models import Form
from vins_core.dm.request import create_request
from personal_assistant.api.personal_assistant import (
    PersonalAssistantAPI, PersonalAssistantAPIError, FAST_BASS_QUERY, FAST_QUERY_BASS_API_URL
)


class BadBassServer(threading.Thread):
    """ Server thread that aceept tcp connections and close it with RST flag """

    def __init__(self):
        super(BadBassServer, self).__init__()

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        sock.bind(('', 0))
        sock.listen(5)
        sock.setblocking(0)

        self.port = sock.getsockname()[1]
        self._sock = sock
        self._stopped = False

    def run(self):
        while not self._stopped:
            try:
                conn, addr = self._sock.accept()

                conn.recv(1024)

                # sending RST
                # https://stackoverflow.com/a/6440364/6870336
                l_onoff = 1
                l_linger = 0
                conn.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER,
                                struct.pack('ii', l_onoff, l_linger))

                conn.close()
            except socket.error as err:
                if err.args[0] == errno.EWOULDBLOCK:
                    time.sleep(0.1)
                else:
                    raise

    def stop(self):
        self._stopped = True
        self.join()
        self._sock.close()


@pytest.fixture(scope='module')
def fake_bass():
    bbs = BadBassServer()
    bbs.start()

    yield 'http://localhost:%s/' % bbs.port
    bbs.stop()


def bass_submit_form_echo_response(request, context):
    request = json.loads(request.body)
    request.pop('meta')
    request['blocks'] = []
    return request


def make_bass_setup_forms_response(response):
    def make_response(request, context):
        return response
    return make_response


def test_bad_bass(fake_bass):
    try:
        requests.get(fake_bass)
    except requests.ConnectionError, exc:
        assert 'Connection reset by peer' in exc


def test_pa_api_submit_form_one_fail(fake_bass, mocker, mock_request, caplog):
    req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': fake_bass})

    orig_url = FAST_QUERY_BASS_API_URL
    path = PersonalAssistantAPI._urls['vins']

    mocker.patch.object(PersonalAssistantAPI, '_get_url_prefix', side_effect=[fake_bass, orig_url])
    mock_request.post(orig_url + path, json=bass_submit_form_echo_response)
    mock_request.post(fake_bass + path, real_http=True)

    form = Form.from_dict({'name': 'test_form'})
    PersonalAssistantAPI().submit_form(
        req_info=req_info,
        form=form,
        action=None,
        balancer_type=FAST_BASS_QUERY,
    )

    assert '''Got a retriable exception ('Connection aborted.', error(104, 'Connection reset by peer')).''' in caplog.text  # noqa


def test_pa_api_submit_form_fail(fake_bass, mocker, mock_request, caplog):
    req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': fake_bass})
    path = PersonalAssistantAPI._urls['vins']

    mocker.patch.object(PersonalAssistantAPI, '_get_url_prefix', side_effect=fake_bass)
    mock_request.post(fake_bass + path, real_http=True)

    form = Form.from_dict({'name': 'test_form'})
    with pytest.raises(PersonalAssistantAPIError):
        PersonalAssistantAPI().submit_form(
            req_info=req_info,
            form=form,
            action=None,
            balancer_type=FAST_BASS_QUERY,
        )


def test_pa_api_setup_forms_one_fail(fake_bass, mocker, mock_request, caplog):
    req_info = create_request(uuid='123', utterance='test')
    orig_url = FAST_QUERY_BASS_API_URL
    path = PersonalAssistantAPI._urls['setup']

    mocker.patch.object(PersonalAssistantAPI, '_get_url_prefix', side_effect=[fake_bass, orig_url])

    forms = [Form('form_1'), Form('form_2')]

    response = {
        'forms': [
            {
                'setup_meta': {
                    'is_feasible': False,
                    'other_key': 'other_value'
                },
                'report_data': {
                    'form': {
                        'name': forms[0].name,
                        'slots': []
                    },
                    'form_1_test_key': 'form_1_test_value'
                }
            },
            {
                'setup_meta': {
                    'is_feasible': False,
                    'other_key': 'other_value'
                },
                'report_data': {
                    'form': {
                        'name': forms[1].name,
                        'slots': [{'name': 'test_slot_2', 'type': 'string'}]
                    }
                }
            }
        ],
        'shared_report_data': {'shared_test_key': 'shared_test_value'}
    }

    mock_request.post(orig_url + path, json=make_bass_setup_forms_response(response))
    mock_request.post(fake_bass + path, real_http=True)

    result = PersonalAssistantAPI().setup_forms(req_info=req_info, forms=forms, balancer_type=FAST_BASS_QUERY)

    assert len(result.forms) == 2

    assert result.forms[0].meta.is_feasible == response['forms'][0]['setup_meta']['is_feasible']
    assert result.forms[0].info.name == response['forms'][0]['report_data']['form']['name']
    assert len(result.forms[0].info.slots) == 0
    assert result.forms[0].precomputed_data == {
        'form_1_test_key': response['forms'][0]['report_data']['form_1_test_key'],
        'shared_test_key': response['shared_report_data']['shared_test_key']
    }

    assert result.forms[1].meta.is_feasible == response['forms'][1]['setup_meta']['is_feasible']
    assert result.forms[1].info.name == response['forms'][1]['report_data']['form']['name']
    assert len(result.forms[1].info.slots) == 1
    assert result.forms[1].info.slots[0].name == response['forms'][1]['report_data']['form']['slots'][0]['name']
    assert result.forms[1].precomputed_data == response['shared_report_data']

    assert '''Got a retriable exception ('Connection aborted.', error(104, 'Connection reset by peer')).''' in caplog.text  # noqa


def test_get_bass_url():
    req_info = create_request(
        uuid='123',
        utterance='test',
        srcrwr={
            'test_src': 'http://foo.com:3020',
            'another_src': 'http://bar.com?data=true',
            'BASS': 'bass.ya.ru:8080'
        }
    )
    expected = 'http://bass.ya.ru:8080/vins?srcrwr=another_src%3Ahttp%3A%2F%2Fbar.com%3Fdata%3Dtrue&srcrwr=test_src%3Ahttp%3A%2F%2Ffoo.com%3A3020'  # noqa
    assert PersonalAssistantAPI._get_bass_url(req_info, None, 'vins') == expected
