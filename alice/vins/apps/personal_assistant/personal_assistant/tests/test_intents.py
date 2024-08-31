# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest

from personal_assistant import intents
from vins_core.dm.request import ReqInfo, AppInfo, Experiments
from vins_core.utils.datetime import utcnow
from vins_core.utils.misc import gen_uuid_for_tests


EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY = 'EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY'
EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES = {'VALUE_A', 'VALUE_B'}
QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY = 'QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY'
QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES = {'INVERSE_VALUE_A', 'INVERSE_VALUE_B'}
FORBIDDEN_INTENTS = {'VALUE_C'}

intents.EXPERIMENT_TO_FORBIDDEN_INTENTS = {EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY: EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES}
intents.QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS = {
    QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY: QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES}
intents.FORBIDDEN_INTENTS = FORBIDDEN_INTENTS

SMART_SPEAKER_APP_INFO = AppInfo(
    app_id='ru.yandex.quasar'
)


def make_req_info(app_info=None, experiments=None, scenario_id=None, semantic_frames=None):
    return ReqInfo(
        request_id=str(gen_uuid_for_tests()),
        client_time=utcnow(),
        uuid=str(gen_uuid_for_tests()),
        app_info=app_info or AppInfo(),
        experiments=Experiments(experiments or {}),
        scenario_id=scenario_id,
        semantic_frames=semantic_frames or [],
    )


def test_get_forbidden_intents():
    expected = {'my_fist_intent', 'my_second_intent'}
    expected.update(FORBIDDEN_INTENTS)

    req_info = make_req_info(experiments={
        'forbidden_intents': ','.join(expected),
    })

    actual = intents.get_forbidden_intents(req_info)
    assert actual == expected


def test_get_forbidden_intents_with_quasar_experiments():
    expected = {'from_request'}
    expected.update(FORBIDDEN_INTENTS)

    req_info = make_req_info(SMART_SPEAKER_APP_INFO, experiments={
        'forbidden_intents': ','.join(expected),
        EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY: '1',
        QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY: '1',
    })
    expected.update(EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES)

    actual = intents.get_forbidden_intents(req_info)
    assert actual == expected


def test_get_forbidden_intents_with_inverse_quasar_experiments():
    expected = {'from_request'}
    expected.update(FORBIDDEN_INTENTS)

    req_info = make_req_info(SMART_SPEAKER_APP_INFO, experiments={
        'forbidden_intents': ','.join(expected),
    })
    expected.update(QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES)

    actual = intents.get_forbidden_intents(req_info)
    assert actual == expected


def test_get_forbidden_intents_with_all_quasar_experiments():
    expected = {'from_request'}
    expected.update(FORBIDDEN_INTENTS)

    req_info = make_req_info(SMART_SPEAKER_APP_INFO, experiments={
        EXPERIMENT_TO_FORBIDDEN_INTENTS_KEY: '1',
        'forbidden_intents': ','.join(expected),
    })
    expected.update(EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES)
    expected.update(QUASAR_INVERSE_EXPERIMENT_TO_FORBIDDEN_INTENTS_VALUES)

    actual = intents.get_forbidden_intents(req_info)
    assert actual == expected


@pytest.mark.parametrize('intent_name, experiments, can_continue', [
    ('personal_assistant.scenarios.get_weather', {}, False),
    ('personal_assistant.scenarios.get_weather', {'vins_use_continue' : 1}, True),
    ('personal_assistant.scenarios.create_reminder', {'vins_use_continue': 1}, False),
    ('personal_assistant.scenarios.search', {'vins_use_continue': 1}, False),
    ('personal_assistant.scenarios.search', {'vins_use_continue': 1, 'vins_continue_on_search': 1}, True),
    ('personal_assistant.scenarios.get_news', {'vins_use_continue': 1}, False),
    ('personal_assistant.scenarios.get_news', {'vins_use_continue': 1, 'vins_continue_on_search': 1}, True),
    ('personal_assistant.scenarios.get_weather', {'vins_use_continue': 1, 'vins_continue_heavy_only': 1}, True),
    ('personal_assistant.scenarios.image_what_is_this', {'vins_use_continue': 1, 'vins_continue_heavy_only': 1}, True),
    ('personal_assistant.internal.hardcoded', {'vins_use_continue': 1, 'vins_continue_heavy_only': 1}, False),
    ('personal_assistant.scenarios.image_what_is_this',
     {'vins_use_continue': 1, 'vins_continue_only_intent': 'personal_assistant.scenarios.get_weather'}, False),
    ('personal_assistant.scenarios.get_weather',
     {'vins_use_continue': 1, 'vins_continue_only_intent': 'personal_assistant.scenarios.get_weather'}, True)
])
def test_can_use_continue_stage(intent_name, experiments, can_continue):
    req_info = make_req_info(experiments=experiments)
    assert intents.can_use_continue_stage(intent_name, req_info) == can_continue


@pytest.mark.parametrize('scenario_id, intent_name, experiment_result, is_irrelevant', [
    (None, 'personal_assistant.scenarios.call', False, True),
    (None, 'personal_assistant.handcrafted.cancel', False, True),
    (None, 'personal_assistant.handcrafted.fast_cancel', False, True),
    (None, 'personal_assistant.scenarios.player_pause', False, True),
    (None, 'personal_assistant.scenarios.sound_louder', False, True),
    (None, 'personal_assistant.scenarios.sound_quiter', False, True),
    (None, 'personal_assistant.scenarios.sound_set_level', False, True),
    (None, 'personal_assistant.scenarios.sound_get_level', False, True),
    (None, 'personal_assistant.scenarios.sound_mute', False, True),
    (None, 'personal_assistant.scenarios.sound_unmute', False, True),
    (None, 'show_traffic', False, False),
    (None, 'show_traffic', False, False),
    (None, 'show_traffic', True, True),
    (None, 'personal_assistant.scenarios.external_skill', False, True),
    (None, 'personal_assistant.scenarios.common.irrelevant', False, True),
    (None, 'personal_assistant.scenarios.common.irrelevant_suffix', False, True),
    ('', 'personal_assistant.scenarios.external_skill', False, True),
    ('new_scenario', 'personal_assistant.scenarios.common.external_skill', False, True),
    ('new_scenario', 'personal_assistant.scenarios.common.irrelevant_suffix', False, True),
    ('new_scenario', 'personal_assistant.scenarios.get_weather', True, True),
    ('show_traffic', 'personal_assistant.scenarios.show_traffic', False, False),
    ('show_traffic', 'personal_assistant.scenarios.show_traffic', True, False),
    ('show_traffic', 'personal_assistant.scenarios.get_weather', False, True),
    ('show_traffic', 'personal_assistant.scenarios.common.irrelevant', False, True),
    ('show_traffic', 'personal_assistant.scenarios.common.irrelevant_suffix', False, True),
    ('show_traffic', 'personal_assistant.feedback.feedback_negative', False, False),
    ('show_traffic', 'personal_assistant.feedback.feedback_positive', False, False),
    ('show_traffic', 'personal_assistant.feedback.feedback_neutral', False, True),
    ('show_traffic', 'personal_assistant.scenarios.search', False, True),
    ('show_traffic', 'personal_assistant.scenarios.search__ellipsis', False, True),
])
def test_is_irrelevant_intent(scenario_id, intent_name, experiment_result, is_irrelevant):
    req_info = make_req_info(scenario_id=scenario_id)

    with mock.patch('personal_assistant.intents.is_irrelevant_intent_by_exp', return_value=experiment_result):
        assert intents.is_irrelevant_intent(intent_name, req_info) == is_irrelevant


@pytest.mark.parametrize('intent_name, experiments, is_irrelevant', [
    ('personal_assistant.handcrafted.cancel', {'vins_pause_commands_relevant_again': 1}, False),
    ('personal_assistant.handcrafted.fast_cancel', {'vins_pause_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.player_pause', {'vins_pause_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_louder', {'vins_sound_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_quiter', {'vins_sound_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_set_level', {'vins_sound_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_get_level', {'vins_sound_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_mute', {'vins_sound_commands_relevant_again': 1}, False),
    ('personal_assistant.scenarios.sound_unmute', {'vins_sound_commands_relevant_again': 1}, False),
    ('show_traffic', {}, False),
    ('show_traffic', {'irrelevant_intents': 'show_traffic_details_exp,show_traffic'}, False),
    ('show_traffic', {'vins_irrelevant_intents': 'show_traffic_details_exp'}, False),
    ('show_traffic', {'vins_irrelevant_intents': 'show_traffic_details_exp,show_traffic'}, True),
    ('show_traffic', {'vins_irrelevant_intents': 'show_traffic_details_exp%2Cshow_traffic'}, True),
    ('show_traffic', {'vins_add_irrelevant_intents=show_traffic_details_exp': 1}, False),
    ('show_traffic', {'vins_add_irrelevant_intents%3Dshow_traffic_details_exp': 1}, False),
    ('show_traffic',
     {'vins_add_irrelevant_intents%3Dshow_traffic_details_exp': 1, 'vins_add_irrelevant_intents%3Dshow_traffic': 1},
     True),
    ('show_traffic',
     {'vins_add_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic_ellipsis': 1,
      'vins_add_irrelevant_intents%3Dshow_traffic': 1},
     True),
    ('show_traffic',
     {'vins_add_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic': 1,
      'vins_add_irrelevant_intents%3Dshow_traffic_ellipsis': 1},
     True),
    ('show_traffic',
     {'vins_add_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic_exp': 1,
      'vins_add_irrelevant_intents%3Dshow_traffic_ellipsis': 1},
     False),
    ('show_traffic',
     {'vins_add_irrelevant_intents=show_traffic_details_exp': 1, 'vins_add_irrelevant_intents=show_traffic': 1},
     True),
])
def test_is_irrelevant_intent_by_exp(intent_name, experiments, is_irrelevant):
    assert intents.is_irrelevant_intent_by_exp(intent_name, Experiments(experiments)) == is_irrelevant


@pytest.mark.parametrize('intent_name, experiments, is_relevant', [
    ('show_traffic', {}, False),
    ('show_traffic', {'vins_remove_irrelevant_intents=show_traffic_details_exp': 1}, False),
    ('show_traffic', {'vins_remove_irrelevant_intents%3Dshow_traffic_details_exp': 1}, False),
    ('show_traffic',
     {'vins_remove_irrelevant_intents%3Dshow_traffic_details_exp': 1, 'vins_remove_irrelevant_intents%3Dshow_traffic': 1},
     True),
    ('show_traffic',
     {'vins_remove_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic_ellipsis': 1,
      'vins_remove_irrelevant_intents%3Dshow_traffic': 1},
     True),
    ('show_traffic',
     {'vins_remove_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic': 1,
      'vins_remove_irrelevant_intents%3Dshow_traffic_ellipsis': 1},
     True),
    ('show_traffic',
     {'vins_remove_irrelevant_intents%3Dshow_traffic_details_exp%2Cshow_traffic_exp': 1,
      'vins_remove_irrelevant_intents%3Dshow_traffic_ellipsis': 1},
     False),
    ('show_traffic',
     {'vins_remove_irrelevant_intents=show_traffic_details_exp': 1, 'vins_remove_irrelevant_intents=show_traffic': 1},
     True),
    ('show_traffic',
     {'vins_remove_irrelevant_intents=show_traffic_details_exp,show_traffic': 1},
     True),
])
def test_is_relevant_intent_by_exp(intent_name, experiments, is_relevant):
    assert intents.is_relevant_intent_by_exp(intent_name, Experiments(experiments)) == is_relevant


@pytest.mark.parametrize('scenario_name, product_scenario_name', [
    ('show_traffic', 'show_traffic'),
    ('foo', None),
    (None, None),
])
def test_product_scenario_name(scenario_name, product_scenario_name):
    req_info = make_req_info(scenario_id=scenario_name)
    assert intents.product_scenario_name(req_info) == product_scenario_name


def test_is_external_only_frames():
    req_info = make_req_info(
        experiments={'vins_external_only_intents=test1,test2': 1},
        semantic_frames=[{'name': 'test1'}, {'name': 'test3'}],
    )

    assert intents.allow_external_only_frame('test1', req_info)
    assert not intents.allow_external_only_frame('test2', req_info)
    assert intents.allow_external_only_frame('test3', req_info)

    req_info = make_req_info(
        experiments={},
        semantic_frames=[{'name': 'test1'}, {'name': 'test3'}],
    )

    assert intents.allow_external_only_frame('test1', req_info)
    assert intents.allow_external_only_frame('test2', req_info)
    assert intents.allow_external_only_frame('test3', req_info)

    req_info = make_req_info(
        experiments={'vins_external_only_intents=test1,test2': 1},
        semantic_frames=[],
    )

    assert not intents.allow_external_only_frame('test1', req_info)
    assert not intents.allow_external_only_frame('test2', req_info)
    assert intents.allow_external_only_frame('test3', req_info)
