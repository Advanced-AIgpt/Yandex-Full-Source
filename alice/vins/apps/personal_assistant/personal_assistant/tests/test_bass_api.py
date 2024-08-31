# coding: utf-8
from __future__ import unicode_literals

import pytest

from personal_assistant.api.personal_assistant import PersonalAssistantAPI, FAST_BASS_QUERY
from vins_core.config.app_config import load_app_config
from vins_core.dm.form_filler.models import Form
from vins_core.dm.request import create_request
from vins_core.dm.request_events import RequestEvent, TextInputEvent
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.utils.config import get_bool_setting
from personal_assistant.testing_framework import AppInfo


def get_form(app_cfg, intent_name):
    for intent in app_cfg.intents:
        if intent.name == intent_name:
            return intent.dm
    raise RuntimeError('Form for intent "%s" is not found.' % intent_name)


@pytest.mark.skip(reason="there is no field 'shared_report_data' in bass response anymore, \
    see https://a.yandex-team.ru/review/861126/files/8#file-22425126:L452 \
    consider remove these tests in the near future")
@pytest.mark.no_requests_mock
def test_setup_forms_api_doesnt_raise_any_exception():
    api = PersonalAssistantAPI()
    app_cfg = load_app_config('personal_assistant')

    api.setup_forms(
        req_info=create_request(uuid=gen_uuid_for_tests(), utterance='hello'),
        forms=[get_form(app_cfg, 'personal_assistant.scenarios.get_time')],
        balancer_type=FAST_BASS_QUERY
    )
    api.setup_forms(
        req_info=create_request(uuid=gen_uuid_for_tests(), utterance='hello'),
        forms=[Form.from_dict({'name': 'unknown_form'})],
        balancer_type=FAST_BASS_QUERY
    )


@pytest.mark.no_requests_mock
def test_submit_form_api_doesnt_raise_any_exception():
    api = PersonalAssistantAPI()
    app_cfg = load_app_config('personal_assistant')
    api.submit_form(
        req_info=create_request(uuid=gen_uuid_for_tests(), utterance='hello'),
        form=get_form(app_cfg, 'personal_assistant.scenarios.get_time'),
        action=None,
        balancer_type=FAST_BASS_QUERY
    )


@pytest.mark.api_test
@pytest.mark.no_requests_mock
def test_setup_request():
    # TODO: rewrite this test to check features content

    if not get_bool_setting('API_TESTS'):
        return

    api = PersonalAssistantAPI()
    app_cfg = load_app_config('personal_assistant')

    form = get_form(app_cfg, 'personal_assistant.scenarios.video_play')

    form.get_slot_by_name('search_text').set_value('терминатора', 'string', 'терминатора')
    form.get_slot_by_name('action').set_value('play', 'video_action', 'включи')

    experiments = ['music_video_setup']
    app_info = AppInfo(app_id='ru.yandex.quasar')
    device_state = {"is_tv_plugged_in": True}

    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance='включи терминатора',
        experiments=experiments,
        app_info=app_info,
        device_state=device_state)

    setup_result = api.setup_forms(
        req_info=req_info,
        forms=[form],
        balancer_type=FAST_BASS_QUERY
    )

    assert len(setup_result.forms) == 1
    assert 'video_web_search' in setup_result.forms[0].meta.factors_data
    assert 'documents' in setup_result.forms[0].meta.factors_data['video_web_search']
    assert 'snippets' in setup_result.forms[0].meta.factors_data['video_web_search']
    assert 'wizards' in setup_result.forms[0].meta.factors_data['video_web_search']


def test_asr_utterance_in_bass_request_meta():
    api = PersonalAssistantAPI()

    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance="four wheels",
        event=RequestEvent.from_dict({
            'type': 'voice_input',
            'asr_result': [{
                'confidence': 0.8,
                'utterance': '4 wheels',
                'words': [
                    {'confidence': 1.0, 'value': 'four'},
                    {'confidence': 1.0, 'value': 'wheels'},
                ],
            }],
        }),
    )
    meta = api._get_meta(req_info, session=None)
    assert meta["utterance"] == "four wheels"
    assert meta["asr_utterance"] == "4 wheels"

    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        utterance="text",
        event=TextInputEvent("text"),
    )
    meta = api._get_meta(req_info, session=None)
    assert 'asr_utterance' not in meta


def test_request_start_time_in_bass_request_meta():
    server_time_ms = 1575319063

    api = PersonalAssistantAPI()

    req_info = create_request(
        uuid=gen_uuid_for_tests(),
        additional_options=dict(
            server_time_ms=server_time_ms,
        ),
    )
    meta = api._get_meta(req_info, session=None)
    assert meta['request_start_time'] == 1000 * server_time_ms
