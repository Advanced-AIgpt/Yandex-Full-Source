# coding: utf-8

import pytest
from vins_core.config.app_config import AppConfig
from vins_core.utils.data import load_data_from_file
from vins_core.dm.intent import Intent
from vins_core.dm.form_filler.models import Form
from vins_core.dm.session import Session
from vins_core.dm.request import AppInfo, ReqInfo, Experiments

from personal_assistant.transition_model import create_pa_transition_model
from personal_assistant import intents
from personal_assistant.transition_model_rules import (
    LogicRule, CheckFormSlotValueRule, CheckPrevCurrIntentRule, CheckFormActiveSlots,
    AllowPrevIntentsRule)


INTERNAL_BOOST = 3
ACTIVE_SLOT_ELLIPSIS_BOOST = 10
INTERNAL_SEARCH_BOOST = 1.5
QUASAR_TV_BOOST = 2
QUASAR_VIDEO_MODE_BOOST = 3
STROKA_DESKTOP_BOOST = 1.000001
QUASAR_STOP_BUZZING_BOOST = 1.03
VIDEO_WITHOUT_SCREEN_BOOST = 0.9


def _get_intent(intent_name):
    return Intent(intents.APP_INTENT_PREFIX + intent_name)


STROKA_APP_INFO = AppInfo(
    app_id='winsearchbar',
    app_version='10.00',
    os_version='5.1.1',
    platform='windows'
)


ANDROID_APP_INFO = AppInfo(
    app_id='ru.yandex.searchplugin',
    app_version='10.00',
    os_version='5.1.1',
    platform='android'
)


IOS_APP_INFO = AppInfo(
    app_id='ru.yandex.mobile',
    app_version='10.00',
    os_version='5.1.1',
    platform='iphone'
)


QUASAR_APP_INFO = AppInfo(
    app_id='ru.yandex.quasar.vins_test',
    platform='android',
    os_version='6.0.1',
    app_version='1.0',
)


@pytest.fixture(scope='module')
def transition_model():
    cfg = AppConfig()
    vinsfile = 'personal_assistant/config/Vinsfile.json'
    cfg.parse_vinsfile(vinsfile)
    intents = [Intent.from_config(intent_cfg) for intent_cfg in cfg.intents]
    return create_pa_transition_model(
        intents,
        custom_rules=cfg.custom_rules,
        boosts=dict(
            internal=INTERNAL_BOOST,
            active_slot_ellipsis=ACTIVE_SLOT_ELLIPSIS_BOOST,
            internal_search=INTERNAL_SEARCH_BOOST,
            quasar_tv=QUASAR_TV_BOOST,
            quasar_video_mode=QUASAR_VIDEO_MODE_BOOST,
            stroka_desktop=STROKA_DESKTOP_BOOST,
            quasar_stop_buzzing=QUASAR_STOP_BUZZING_BOOST,
            video_without_screen=VIDEO_WITHOUT_SCREEN_BOOST
        ),
    )


@pytest.fixture(scope='function')
def session():
    return Session(app_id=None, uuid=None)


@pytest.mark.parametrize(('prev_intent_name', 'intent_name', 'expected_score'), [
    # None -> intent
    (None, 'scenarios.get_weather', 1),
    (None, 'general_conversation.general_conversation', 1),
    (None, 'general_conversation.general_conversation_dummy', 1),
    (None, 'scenarios.search', 1),
    (None, 'scenarios.find_poi', 1),
    (None, 'scenarios.show_route', 1),
    (None, 'scenarios.show_route__ellipsis', 0),
    (None, 'scenarios.show_route__show_on_map', 0),
    (None, 'scenarios.find_poi__ellipsis', 0),
    (None, 'scenarios.find_poi__details', 0),
    (None, 'scenarios.find_poi__call', 0),
    (None, 'scenarios.find_poi__open_site', 0),
    (None, 'scenarios.find_poi__scroll__next', 0),
    (None, 'scenarios.find_poi__scroll__prev', 0),
    (None, 'scenarios.find_poi__scroll__by_index', 0),
    (None, 'scenarios.search__serp', 0),
    (None, 'scenarios.get_weather__details', 0),
    (None, 'scenarios.get_weather__ellipsis', 0),
    (None, 'handcrafted.hello', 1),
    (None, 'handcrafted.tell_me_another', 0),

    # intent1 -> intent2
    ('scenarios.get_weather', 'scenarios.get_weather', 1),
    ('scenarios.get_weather', 'scenarios.get_weather__ellipsis', INTERNAL_BOOST),
    ('scenarios.get_weather__ellipsis', 'scenarios.get_weather', 1),
    ('scenarios.get_weather__ellipsis', 'scenarios.get_weather__ellipsis', INTERNAL_BOOST),
    ('scenarios.convert', 'scenarios.convert', 1),
    ('scenarios.convert', 'scenarios.convert__ellipsis', INTERNAL_BOOST),
    ('scenarios.convert__ellipsis', 'scenarios.convert', 1),
    ('scenarios.convert__ellipsis', 'scenarios.convert__ellipsis', INTERNAL_BOOST),
    ('scenarios.show_route', 'scenarios.show_route__ellipsis', INTERNAL_BOOST),
    ('scenarios.find_poi', 'scenarios.find_poi__ellipsis', INTERNAL_BOOST),
    ('scenarios.find_poi__ellipsis', 'scenarios.find_poi', 1),
    ('scenarios.find_poi__ellipsis', 'scenarios.find_poi__ellipsis', INTERNAL_BOOST),
    ('general_conversation.general_conversation', 'general_conversation.general_conversation', 1),  # noqa
    ('general_conversation.general_conversation_dummy', 'general_conversation.general_conversation', 1),  # noqa
    ('general_conversation.general_conversation', 'general_conversation.general_conversation_dummy', 1),  # noqa
    ('general_conversation.general_conversation', 'scenarios.search', 1),
    ('general_conversation.general_conversation_dummy', 'scenarios.search', 1),
    ('scenarios.find_poi', 'scenarios.find_poi__scroll__next', INTERNAL_BOOST),
    ('scenarios.find_poi__ellipsis', 'scenarios.find_poi__scroll__by_index', INTERNAL_BOOST),
    ('scenarios.find_poi__ellipsis', 'scenarios.find_poi__details', INTERNAL_BOOST),
    ('scenarios.find_poi__scroll__prev', 'scenarios.find_poi', 1),
    ('scenarios.find_poi__scroll__by_index', 'scenarios.find_poi__scroll__next', INTERNAL_BOOST),
    ('scenarios.find_poi__details', 'scenarios.find_poi__scroll__next', INTERNAL_BOOST),
    ('scenarios.find_poi__open_site', 'scenarios.find_poi__scroll__next', INTERNAL_BOOST),
    ('scenarios.find_poi__open_site', 'scenarios.find_poi__call', INTERNAL_BOOST),
    ('scenarios.find_poi__open_site', 'scenarios.find_poi__ellipsis', INTERNAL_BOOST),
    ('scenarios.find_poi__scroll__by_index', 'scenarios.find_poi__scroll__prev', INTERNAL_BOOST),
    ('scenarios.find_poi__scroll__by_index', 'scenarios.find_poi', 1),
    ('scenarios.find_poi__details', 'scenarios.find_poi', 1),
    ('scenarios.find_poi__call', 'scenarios.find_poi', 1),
    ('scenarios.search__serp', 'scenarios.search', 1),
    ('scenarios.search', 'scenarios.search', 1),
    ('scenarios.search__serp', 'scenarios.search__serp', INTERNAL_SEARCH_BOOST),
    ('scenarios.search', 'scenarios.search__serp', INTERNAL_SEARCH_BOOST),
    ('handcrafted.tell_me_a_joke', 'handcrafted.tell_me_another', 1),
    ('handcrafted.hello', 'handcrafted.tell_me_another', 0),

    # external_skill state machine
    (None, 'scenarios.external_skill', 1),
    (None, 'scenarios.external_skill__continue', 0),
    (None, 'scenarios.external_skill__deactivate', 0),
    (None, 'scenarios.external_skill__activate_only', 0),

    ('handcrafted.hello', 'scenarios.external_skill', 1),
    ('handcrafted.hello', 'scenarios.external_skill__continue', 0),
    ('handcrafted.hello', 'scenarios.external_skill__deactivate', 0),
    ('handcrafted.hello', 'scenarios.external_skill__activate_only', 0),

    ('scenarios.external_skill', 'handcrafted.hello', 0),
    ('scenarios.external_skill', 'scenarios.external_skill__continue', INTERNAL_BOOST),
    ('scenarios.external_skill', 'scenarios.external_skill__deactivate', INTERNAL_BOOST),
    ('scenarios.external_skill', 'scenarios.external_skill', 0),
    ('scenarios.external_skill', 'scenarios.external_skill__activate_only', 0),

    ('scenarios.external_skill__continue', 'handcrafted.hello', 0),
    ('scenarios.external_skill__continue', 'scenarios.external_skill__continue', INTERNAL_BOOST),
    ('scenarios.external_skill__continue', 'scenarios.external_skill__deactivate', INTERNAL_BOOST),
    ('scenarios.external_skill__continue', 'scenarios.external_skill', 0),
    ('scenarios.external_skill__continue', 'scenarios.external_skill__activate_only', 0),

    ('scenarios.external_skill__deactivate', 'handcrafted.hello', 1),
    ('scenarios.external_skill__deactivate', 'scenarios.external_skill__continue', 0),
    ('scenarios.external_skill__deactivate', 'scenarios.external_skill__deactivate', 0),
    ('scenarios.external_skill__deactivate', 'scenarios.external_skill', 1),
    ('scenarios.external_skill__deactivate', 'scenarios.external_skill__activate_only', 0),

    ('scenarios.external_skill__activate_only', 'handcrafted.hello', 1),
    ('scenarios.external_skill__activate_only', 'scenarios.external_skill__continue', 0),
    ('scenarios.external_skill__activate_only', 'scenarios.external_skill__deactivate', 0),
    ('scenarios.external_skill__activate_only', 'scenarios.external_skill', 1),
    ('scenarios.external_skill__activate_only', 'scenarios.external_skill__activate_only', 0),


    # Multi-step scenario transitions
    (None, 'scenarios.voiceprint_enroll', 1),
    (None, 'scenarios.voiceprint_enroll__collect_voice', 0),
    ('scenarios.voiceprint_enroll', 'scenarios.voiceprint_enroll__collect_voice', INTERNAL_BOOST),
    ('scenarios.voiceprint_enroll__collect_voice', 'scenarios.voiceprint_enroll', 0),
    ('scenarios.voiceprint_enroll__collect_voice', 'scenarios.voiceprint_enroll__finish', INTERNAL_BOOST),
    ('scenarios.voiceprint_enroll__finish', 'scenarios.voiceprint_enroll__collect_voice', 0),
    (None, 'internal.bugreport', 1),
    (None, 'internal.bugreport__continue', 0),
    (None, 'internal.bugreport__deactivate', 0),
    ('internal.bugreport__continue', 'internal.bugreport', 0),
    ('internal.bugreport__deactivate', 'internal.bugreport', 1),
    ('internal.bugreport__deactivate', 'internal.bugreport__continue', 0),
    ('internal.bugreport__deactivate', 'internal.bugreport__deactivate', 0),
    ('internal.bugreport__deactivate', 'scenarios.voiceprint_enroll__collect_voice', 0),
    ('internal.bugreport__deactivate', 'scenarios.get_weather', 1),
    ('internal.bugreport__deactivate', 'scenarios.get_weather__ellipsis', 0),
    ('internal.bugreport__continue', 'scenarios.get_weather', 0),
    ('internal.bugreport__continue', 'scenarios.get_weather__ellipsis', 0),
])
def test_transition_model(session, transition_model, prev_intent_name, intent_name, expected_score):
    if prev_intent_name is not None:
        session.change_intent(_get_intent(prev_intent_name))
    model_score = transition_model(_get_intent(intent_name).name, session)
    assert model_score == expected_score, 'unexpected score for transition %s -> %s' % (
        prev_intent_name, intent_name
    )


@pytest.mark.parametrize(('prev_intent_name', 'intent_name', 'expected_score', 'app_info', 'experiments'), [
    # Stroka intents are blocked for non-stroka clients
    (None, 'stroka.open_folder', 0, ANDROID_APP_INFO, []),
    (None, 'stroka.open_folder', 0, IOS_APP_INFO, []),
    ('scenarios.find_poi', 'stroka.restart_pc', 0, ANDROID_APP_INFO, []),
    ('scenarios.find_poi', 'stroka.restart_pc', 0, IOS_APP_INFO, []),

    # Stroka intents are available for stroka clients
    (None, 'stroka.open_folder', STROKA_DESKTOP_BOOST, STROKA_APP_INFO, []),
    ('scenarios.find_poi', 'stroka.restart_pc', STROKA_DESKTOP_BOOST, STROKA_APP_INFO, []),

    # Video playback is not available for stroka
    (None, 'scenarios.video_play', 0, STROKA_APP_INFO, []),

    # Video playback is available for stroka when video_play flag is on
    (None, 'scenarios.video_play', 1, STROKA_APP_INFO, ['video_play']),

    # Ambient sounds are available when ambient_sound flag is on and not in add_forbidden_intent
    (None, 'scenarios.music_ambient_sound', 1, QUASAR_APP_INFO, ['ambient_sound']),
    (None, 'scenarios.music_ambient_sound', 0, QUASAR_APP_INFO, ['ambient_sound', 'add_forbidden_intent=personal_assistant.scenarios.music_ambient_sound']),


    # Boosts are preserved after multistep scenarios
    ('internal.bugreport__deactivate', 'stroka.open_folder', STROKA_DESKTOP_BOOST, STROKA_APP_INFO, []),

    # Bugreport is multistep scenario for quasar clients
    ('internal.bugreport', 'internal.bugreport', 1, QUASAR_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__continue', INTERNAL_BOOST, QUASAR_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__deactivate', INTERNAL_BOOST, QUASAR_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__continue', INTERNAL_BOOST, QUASAR_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__deactivate', INTERNAL_BOOST, QUASAR_APP_INFO, []),

    # Bugreport is not multistep scenario for non-quasar clients
    ('internal.bugreport', 'internal.bugreport', 1, ANDROID_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport', 1, IOS_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport', 1, STROKA_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__continue', 0, ANDROID_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__continue', 0, IOS_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__continue', 0, STROKA_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__deactivate', 0, ANDROID_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__deactivate', 0, IOS_APP_INFO, []),
    ('internal.bugreport', 'internal.bugreport__deactivate', 0, STROKA_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__continue', 0, ANDROID_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__continue', 0, IOS_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__continue', 0, STROKA_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__deactivate', 0, ANDROID_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__deactivate', 0, IOS_APP_INFO, []),
    ('internal.bugreport__continue', 'internal.bugreport__deactivate', 0, STROKA_APP_INFO, []),
])
def test_transition_model_with_app_info(
    session, transition_model, prev_intent_name, intent_name, expected_score, app_info, experiments
):
    req_info = ReqInfo(
        uuid='test_transition_model',
        client_time=None,
        app_info=app_info,
        experiments=Experiments(experiments),
    )
    if prev_intent_name is not None:
        session.change_intent(_get_intent(prev_intent_name))
    model_score = transition_model(_get_intent(intent_name).name, session, req_info=req_info)
    assert model_score == expected_score


def test_active_slot_ellipsis_boost(session, transition_model):
    form = Form.from_dict({
        'name': 'personal_assistant.scenarios.get_weather',
        'slots': [{
            'name': 'test_slot',
            'type': 'string',
            'optional': True
        }]
    })

    session.change_intent(Intent(form.name))
    session.change_form(form)

    assert transition_model(
        'personal_assistant.scenarios.get_weather__ellipsis', session
    ) == INTERNAL_BOOST

    form.get_slot_by_name('test_slot').active = True
    session.change_form(form)

    assert transition_model(
        'personal_assistant.scenarios.get_weather__ellipsis', session
    ) == INTERNAL_BOOST * ACTIVE_SLOT_ELLIPSIS_BOOST


@pytest.mark.parametrize(('intent_name', 'expected_score', 'is_tv_plugged_in', 'current_screen', 'experiments'), [
    ('scenarios.quasar.goto_video_screen', QUASAR_TV_BOOST, True, 'main', []),
    ('scenarios.quasar.goto_video_screen', 0.0, False, 'main', []),
    ('scenarios.quasar.goto_video_screen', 0.0, True, 'gallery', []),
    ('scenarios.quasar.go_forward', QUASAR_TV_BOOST, True, 'gallery', []),
    ('scenarios.quasar.go_forward', 0.0, False, 'gallery', []),
    ('scenarios.quasar.go_forward', 0.0, True, 'main', []),
    ('scenarios.quasar.go_backward', QUASAR_TV_BOOST, True, 'gallery', []),
    ('scenarios.quasar.go_backward', 0.0, False, 'gallery', []),
    ('scenarios.quasar.go_backward', QUASAR_TV_BOOST, True, 'main', []),
    ('scenarios.quasar.go_home', QUASAR_TV_BOOST, True, 'gallery', []),
    ('scenarios.quasar.go_home', QUASAR_TV_BOOST, False, 'gallery', []),
    ('scenarios.quasar.go_home', QUASAR_TV_BOOST, True, 'main', []),
    ('scenarios.quasar.open_current_video', QUASAR_TV_BOOST, True, 'description', []),
    ('scenarios.quasar.open_current_video', QUASAR_TV_BOOST, True, 'payment', []),
    ('scenarios.quasar.open_current_video', QUASAR_TV_BOOST, True, 'video_player', []),
    ('scenarios.quasar.open_current_video', 0.0, False, 'video_player', []),
    ('scenarios.quasar.open_current_video', 0.0, True, 'main', []),
    ('scenarios.quasar.open_current_video', 0.0, True, 'gallery', []),
    ('scenarios.video_play', 1.0, True, 'main', []),
    ('scenarios.video_play', VIDEO_WITHOUT_SCREEN_BOOST, False, 'main', []),
    ('scenarios.video_play', 1.0, True, 'gallery', []),
    ('scenarios.video_play', 1.0, True, 'main', ['quasar_video_mode_boost']),
    ('scenarios.video_play', QUASAR_VIDEO_MODE_BOOST, True, 'gallery', ['quasar_video_mode_boost']),
    ('scenarios.quasar.open_current_video', QUASAR_VIDEO_MODE_BOOST * QUASAR_TV_BOOST, True, 'description', ['quasar_video_mode_boost']),  # noqa
])
def test_quasar_video_intents(
    session, transition_model, intent_name, expected_score, is_tv_plugged_in, current_screen, experiments
):
    req_info = ReqInfo(
        uuid='test_transition_model',
        client_time=None,
        experiments=Experiments(experiments),
        app_info=QUASAR_APP_INFO,
        device_state={
            'is_tv_plugged_in': is_tv_plugged_in,
            'video': {
                'current_screen': current_screen,
            }
        }
    )

    model_score = transition_model(_get_intent(intent_name).name, session, req_info=req_info)
    assert model_score == expected_score


@pytest.mark.parametrize(('intent_name', 'expected_score', 'is_alarm_buzzing', 'is_timer_buzzing'), [
    ('scenarios.alarm_stop_playing', 0, False, False),
    ('scenarios.timer_stop_playing', 0, False, False),
    ('scenarios.alarm_stop_playing', 0, False, True),
    ('scenarios.timer_stop_playing', 0, True, False),
    ('scenarios.alarm_stop_playing', QUASAR_STOP_BUZZING_BOOST, True, False),
    ('scenarios.timer_stop_playing', QUASAR_STOP_BUZZING_BOOST, False, True),
    ('scenarios.alarm_stop_playing', QUASAR_STOP_BUZZING_BOOST, True, True),
    ('scenarios.timer_stop_playing', QUASAR_STOP_BUZZING_BOOST, True, True),
])
def test_stop_buzzer_intents(
    session, transition_model, intent_name, expected_score, is_alarm_buzzing, is_timer_buzzing,
):
    req_info = ReqInfo(
        uuid='test_transition_model',
        client_time=None,
        experiments=Experiments({'enable_timers_alarms': '1'}),
        app_info=QUASAR_APP_INFO,
        device_state={
            'alarm_state': {
                'currently_playing': is_alarm_buzzing,
            },
            'timers': {
                'active_timers': [
                    {
                        'currently_playing': is_timer_buzzing,
                    },
                ]
            }
        }
    )
    model_score = transition_model(_get_intent(intent_name).name, session, req_info=req_info)
    assert model_score == expected_score


@pytest.mark.parametrize(('session_intent', 'intent_name', 'prev_intent', 'curr_intent', 'boost', 'result'), [
    ('scenarios.video_play', 'scenarios.video_play__cancel', 'scenarios.video_play', 'scenarios.video_play__ellipsis', 2.0, False),  # noqa
    ('scenarios.video_play', 'scenarios.video_play__ellipsis', 'scenarios.video_play', 'scenarios.video_play__cancel', 1.4, False),  # noqa
    ('scenarios.video_play', 'scenarios.video_play__ellipsis', 'scenarios.video_play__cancel', 'scenarios.video_play', 1.4, False),  # noqa
    ('scenarios.quasar.open_current_video', 'scenarios.quasar.go_home', 'scenarios.quasar.open_current_video', 'scenarios.quasar.go_home', 2, True),  # noqa
])
def test_prev_curr_intent_rule(session, session_intent, intent_name, prev_intent, curr_intent, boost, result):
    form = Form.from_dict({
        'name': session_intent,
        'slots': []
    })
    session.change_intent(Intent(form.name))
    session.change_form(form)
    rule = CheckPrevCurrIntentRule(prev_intent, curr_intent, boost=boost)
    assert rule.check(intent_name, session.intent_name, form) == result
    assert rule.get_boost(intent_name, session.intent_name, form) == boost


def test_form_active_slots(session):
    data = load_data_from_file('personal_assistant/tests/test_data/active_form_slots_rule_test.json')
    for (form, intent_name, prev_intent, curr_intent, boost, result) in data:
        if form:
            form = Form.from_dict(form)
            session.change_intent(Intent(form.name))
            session.change_form(form)
        rule = CheckFormActiveSlots(prev_intent, curr_intent, boost=boost)
        assert rule.check(intent_name, session.intent_name, form) == result
        assert rule.get_boost(intent_name, session.intent_name, form) == boost


def test_form_slot_value(session):
    data = load_data_from_file('personal_assistant/tests/test_data/form_slots_value_rule_test.json')
    for (form, slot, slot_value_key, value, boost, result) in data:
        if form:
            form = Form.from_dict(form)
            session.change_intent(Intent(form.name))
            session.change_form(form)
        rule = CheckFormSlotValueRule(
            slot=slot,
            value=value,
            slot_value_key=slot_value_key,
            boost=boost,
        )
        assert rule.check('', session.intent_name, form) == result
        assert rule.get_boost('', session.intent_name, form) == boost


def test_logic_rule(session):
    data = load_data_from_file('personal_assistant/tests/test_data/logic_rules_test.json')
    for (form, intent, operation, children, boost, result) in data:
        if form:
            form = Form.from_dict(form)
            session.change_intent(Intent(form.name))
            session.change_form(form)
        rule = LogicRule(operation, children, boost=boost)
        assert rule.check(intent, session.intent_name, form) == result
        assert rule.get_boost(intent, session.intent_name, form) == boost


@pytest.mark.parametrize(('session_intent', 'intent_name', 'prev_intents', 'curr_intent', 'boost', 'result'), [
    ('scenarios.alarm_show', 'scenarios.video_play', ['scenarios.alarm_show'], 'scenarios.common.list_cancel', 1.0, False),  # noqa
    ('scenarios.alarm_show', 'scenarios.common.list_cancel', ['scenarios.alarm_show'], 'scenarios.common.list_cancel', 1.0, True),  # noqa
    ('scenarios.video_play', 'scenarios.common.list_cancel', ['scenarios.alarm_show'], 'scenarios.common.list_cancel', 0.0, True),  # noqa
])
def test_allow_prev_intent_rule(session, session_intent, intent_name, prev_intents, curr_intent, boost, result):
    form = Form.from_dict({
        'name': session_intent,
        'slots': []
    })
    session.change_intent(Intent(form.name))
    session.change_form(form)
    rule = AllowPrevIntentsRule(prev_intents, curr_intent, boost=1)
    assert rule.check(intent_name, session.intent_name, form) == result
    assert rule.get_boost(intent_name, session.intent_name, form) == boost
