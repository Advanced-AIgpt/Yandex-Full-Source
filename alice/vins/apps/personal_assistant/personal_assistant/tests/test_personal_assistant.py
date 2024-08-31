# coding: utf-8
from __future__ import unicode_literals

import pytest
import requests_mock

from uuid import uuid4

from vins_core.dm.form_filler.models import Form
from vins_core.dm.request import create_request
from personal_assistant.api.personal_assistant import (
    PersonalAssistantAPI,
    PersonalAssistantAPIError,
    FAST_BASS_QUERY)

from personal_assistant.testing_framework import (
    user_context_fail_mock,
    user_context_mock,
    check_meta_mock
)


@pytest.fixture(scope='module')
def pa_api():
    return PersonalAssistantAPI()


def test_custom_bass_url(pa_api):
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)


def test_bass_joker(pa_api):
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        test_joker = 'prj=vins&test=weather'
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        req_info.additional_options['joker'] = test_joker
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
        history = mock.request_history
        assert len(history) == 1
        assert history[0].headers['x-yandex-proxy-header-x-yandex-joker'] == test_joker


def test_bass_joker_proxy(pa_api):
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        test_joker_proxy = 'localhost:12345'
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        req_info.additional_options['joker_proxy'] = test_joker_proxy
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
        history = mock.request_history
        assert len(history) == 1
        assert history[0].headers['x-yandex-via-proxy'] == test_joker_proxy


def test_bass_joker_proxy_via_env(monkeypatch, pa_api):
    test_joker_proxy = 'localhost:12345'
    monkeypatch.setenv('JOKER_MOCKER_PROXY', test_joker_proxy)
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
        history = mock.request_history
        assert len(history) == 1
        assert history[0].headers['x-yandex-via-proxy'] == test_joker_proxy


def test_bass_joker_proxy_header_priority(monkeypatch, pa_api):
    monkeypatch.setenv('JOKER_MOCKER_PROXY', 'localhost:23456')
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        test_joker_proxy = 'localhost:12345'
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        req_info.additional_options['joker_proxy'] = test_joker_proxy
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
        history = mock.request_history
        assert len(history) == 1
        assert history[0].headers['x-yandex-via-proxy'] == test_joker_proxy


def test_proxy_header(pa_api):
    bass_url = 'http://bass.ai.yandex.net/'
    form = Form.from_dict({'name': 'test_form'})
    with requests_mock.Mocker() as mock:
        mock.post(bass_url + 'vins', json={'form': {'name': 'test_form', 'slots': []}})
        test_header = 'HEADER'
        test_header_value = 'VALUE'
        req_info = create_request(uuid='123', utterance='test', srcrwr={'BASS': bass_url})
        req_info.proxy_header[test_header + '1'] = test_header_value + '1'
        req_info.proxy_header[test_header + '2'] = test_header_value + '2'
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
        history = mock.request_history
        assert len(history) == 1
        assert history[0].headers[test_header + '1'] == test_header_value + '1'
        assert history[0].headers[test_header + '2'] == test_header_value + '2'


def test_personal_assistant_saveload(pa_api):
    uuid = uuid4()
    with user_context_mock():
        pa_api.save_value(req_info=create_request(uuid=uuid), uuid=uuid, key='test_key', value='test_value')
        assert pa_api.load_value(req_info=create_request(uuid=uuid), uuid=uuid, key='test_key') == 'test_value'
        assert pa_api.load_value(req_info=create_request(uuid=uuid), uuid=uuid, key='test_key_unknown') is None


def test_personal_assistant_fail(pa_api):
    uuid = uuid4()
    with user_context_fail_mock(), pytest.raises(PersonalAssistantAPIError):
        pa_api.load_value(req_info=create_request(uuid=uuid), uuid=uuid, key='test_key_unknown')


def test_meta_with_none_device_id(pa_api):
    form = Form.from_dict({'name': 'test_form'})
    with check_meta_mock(dict(device_id=None, uuid='123')):
        req_info = create_request(uuid='123')
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)


def test_meta_with_device_id(pa_api):
    form = Form.from_dict({'name': 'test_form'})
    device_id = uuid4()
    with check_meta_mock(dict(device_id=device_id.hex, uuid='123')):
        req_info = create_request(uuid='123', device_id=device_id)
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)


def test_meta_with_device_state(pa_api):
    form = Form.from_dict({'name': 'test_form'})
    with check_meta_mock(dict(device_state={'sound_level': 5}, uuid='123')):
        req_info = create_request(uuid='123', device_state={'sound_level': 5})
        pa_api.submit_form(req_info=req_info, form=form, action=None, balancer_type=FAST_BASS_QUERY)
