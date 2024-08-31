# coding: utf-8
from __future__ import unicode_literals

import datetime
import functools
import json
import logging
from copy import deepcopy
from uuid import uuid4 as gen_uuid

import re
import emoji
import mock
import numpy as np
import pytest
import pytz
import requests_mock
from freezegun import freeze_time
from PIL import Image
from StringIO import StringIO

from vins_core.dm.form_filler.models import Form
from vins_core.dm.intent import Intent
from vins_core.dm.request import AppInfo, ReqInfo, create_request, Experiments
from vins_core.dm.request_events import SuggestedInputEvent, VoiceInputEvent, ServerActionEvent
from vins_core.dm.session import Session
from vins_core.ext.general_conversation import gc_mock, gc_fail_mock
from vins_core.common.utterance import Utterance
from vins_core.utils.data import load_data_from_file
from vins_core.utils.datetime import parse_tz, utcnow
from vins_core.utils.misc import all_subsets
from vins_core.utils.operators import item

from personal_assistant import intents
from personal_assistant.api.personal_assistant import SLOW_BASS_QUERY, FAST_BASS_QUERY
from personal_assistant.testing_framework import (
    form_handling_mock,
    convert_for_assert,
    geo_search_api_mock,
    poi_search_api_mock,
    poi_search_api_mock_nothing_found,
    form_handling_fail_mock,
    poi_opening_hours,
    poi_opening_hours_round_the_clock,
    current_weather_api_mock,
    weather_for_day_api_mock,
    weather_for_range_api_mock,
    search_api_mock,
    show_route_api_mock,
    traffic_api_mock,
    suggests_mock,
    bass_action_mock,
    general_conversation_mock as bass_general_conversation_mock,
    cards_mock,
    poi_opening_hours_list,
    universal_form_mock,
    phone_call_mock,
    check_meta_mock
)
from personal_assistant.general_conversation import GC_CONFIG

moscow = parse_tz('Europe/Moscow')

TEST_REQUEST_ID = 'test_dialog-request-id'


@pytest.fixture
def f(vins_app):
    return functools.partial(vins_app.handle_utterance, str(gen_uuid()))


@pytest.fixture
def s(vins_app):
    return functools.partial(vins_app.handle_utterance, str(gen_uuid()), text_only=False, request_id=TEST_REQUEST_ID)


@pytest.fixture
def handle_semantic_frames(vins_app):
    return functools.partial(vins_app.handle_semantic_frames, str(gen_uuid()))


@pytest.fixture
def handle_event(vins_app):
    return functools.partial(vins_app.handle_event, str(gen_uuid()), text_only=False)


@pytest.fixture
def fs(vins_app):
    def text_and_voice(uuid, utterance):
        response = vins_app.handle_utterance(uuid, utterance, text_only=False)
        return response['voice_text'], response['cards'][0]['text']

    return functools.partial(text_and_voice, str(gen_uuid()))


@pytest.fixture
def c(vins_app):
    return functools.partial(vins_app.handle_callback, str(gen_uuid()), text_only=False)


@pytest.fixture
def handle_meta():
    def get_meta_without_analytics_info(meta):
        result = [x for x in meta if x.get('type') != 'analytics_info']
        assert len(result) + 1 == len(meta), 'Meta must contain analytics_info'
        return result

    return get_meta_without_analytics_info


@pytest.fixture
def handle_response(handle_meta):
    def get_response_without_analytics_info(response):
        response['meta'] = handle_meta(response['meta'])
        return response

    return get_response_without_analytics_info


def _city_cases(prepositional, preposition='в'):
    return {
        'city_cases': {
            'preposition': preposition,
            'prepositional': prepositional
        }
    }


def _suggest_logging_action(caption, suggest_type, uri=None, utterance=None,
                            user_utterance=None, block_data=None, form_update=None, request_id=TEST_REQUEST_ID):
    result = {
        'type': 'server_action',
        'name': 'on_suggest',
        'ignore_answer': True,
        'payload': {
            'suggest_block': {
                'form_update': form_update,
                'data': block_data,
                'suggest_type': suggest_type,
                'type': 'suggest'
            },
            'caption': caption,
            'request_id': request_id,
        }
    }

    if uri:
        result['payload']['uri'] = uri
    if utterance:
        result['payload']['utterance'] = utterance
    if user_utterance:
        result['payload']['user_utterance'] = user_utterance

    return result


def _type_silent_action(text):
    return {
        'type': 'client_action',
        'name': 'type_silent',
        'sub_name': 'render_buttons_type_silent',
        'payload': {
            'text': text,
        }
    }


def _form_update_action(form_name):
    return {
        'name': 'update_form',
        'payload': {
            'form_update': {
                'name': form_name
            },
            'resubmit': False,
        },
        'type': 'server_action',
        'ignore_answer': False,
    }


def _feedback_suggest(is_positive, request_id=TEST_REQUEST_ID):
    thumb = emoji.emojize(':thumbsup:' if is_positive else ':thumbsdown:', use_aliases=True)
    caption = thumb
    suggest_type = 'feedback__positive' if is_positive else 'feedback__negative'
    form_name = intents.FEEDBACK_POSITIVE if is_positive else intents.FEEDBACK_NEGATIVE
    return {
        'title': caption,
        'type': 'action',
        'directives': [
            _type_silent_action(caption),
            _form_update_action(form_name),
            _suggest_logging_action(
                caption=caption,
                user_utterance=caption,
                suggest_type=suggest_type,
                form_update={'name': form_name},
                request_id=request_id,
            ),
        ]
    }


def _search_internet_fallback_suggest(query):
    caption = '%s "%s"' % (emoji.emojize(':mag:', use_aliases=True), query)
    return {
        'title': caption,
        'type': 'action',
        'directives': [
            _type_silent_action(caption),
            _suggest_logging_action(
                caption=caption,
                user_utterance=caption,
                suggest_type='search_internet_fallback',
            ),
        ]
    }


def _assert_no_suggest(response, suggest_name):
    for suggest in response['suggests']:
        for directive in suggest['directives']:
            if directive['name'] != 'on_suggest':
                continue
            assert directive['payload']['suggest_block']['suggest_type'] != suggest_name


def test_random_state(f):
    with suggests_mock([]):
        rstate = np.random.get_state()
        f('привет')
        rnd1 = np.random.randint(10)
        np.random.set_state(rstate)
        f('привет')
        rnd2 = np.random.randint(10)
        np.random.set_state(rstate)
        f('привет')
        rnd3 = np.random.randint(10)

        assert rnd1 == rnd2 == rnd3


class TestFeedback:
    NEGATIVE_SUGGESTS_NAMES = [
        'feedback_negative__all_good',
        'feedback_negative__asr_error',
        'feedback_negative__bad_answer',
        'feedback_negative__offensive_answer',
        'feedback_negative__other',
        'feedback_negative__tts_error'
    ]

    def test_positive(self, c):
        with suggests_mock([]):
            res = c(
                'update_form',
                callback_args={
                    'form_update': {'name': 'personal_assistant.feedback.feedback_positive'},
                    'resubmit': False,
                }
            )
            assert res['voice_text'] == 'Спасибо за поддержку!'

    def test_negative(self, c):
        with suggests_mock(self.NEGATIVE_SUGGESTS_NAMES):
            res = c(
                'update_form',
                callback_args={
                    'form_update': {'name': 'personal_assistant.feedback.feedback_negative'},
                    'resubmit': False,
                }
            )

            assert res['voice_text']
            assert not res['directives']
            assert not any(c.get('buttons') for c in res['cards'])

            for vins_suggest, correct_suggest_name in zip(res['suggests'], self.NEGATIVE_SUGGESTS_NAMES):
                assert vins_suggest['directives'][-1]['payload']['suggest_block']['suggest_type'] == correct_suggest_name  # noqa

    @pytest.mark.parametrize('feedback_name', [
        'feedback_positive',
        'feedback_negative',
        'feedback_negative__all_good',
        'feedback_negative__asr_error',
        'feedback_negative__bad_answer',
        'feedback_negative__offensive_answer',
        'feedback_negative__other',
        'feedback_negative__tts_error'
    ])
    def test_context_restoration(self, vins_app, handle_meta, feedback_name):
        uuid = str(gen_uuid())

        with current_weather_api_mock(temperature=-1, condition='снег'):
            assert vins_app.handle_utterance(uuid, 'расскажи погоду') == 'Сейчас в Москве -1, снег.'

        form_name = 'personal_assistant.feedback.' + feedback_name
        res = vins_app.handle_callback(
            uuid,
            callback_name='update_form',
            callback_args={'form_update': {'name': form_name}},
            text_only=False
        )
        assert handle_meta(res['meta']) == [
            {
                'type': 'form_restored',
                'overriden_form': form_name
            }
        ]

        spb = _city_cases('Санкт-Петербурге')
        with current_weather_api_mock(
            temperature=-3,
            condition='как всегда ветренно',
            location=spb,
        ):
            assert vins_app.handle_utterance(uuid, 'а в питере?') == (
                'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.'
            )


class TestWeather:
    @freeze_time('2016-10-26 21:13')
    def test_formfilling(self, f):
        with current_weather_api_mock(temperature=-1, condition='снег'):
            assert f('расскажи погоду') == 'Сейчас в Москве -1, снег.'

        spb = _city_cases('Санкт-Петербурге')
        with current_weather_api_mock(
                temperature=-3,
                condition='как всегда ветренно',
                location=spb,
        ):
            assert f('а в питере?') == (
                'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.')

        dt = datetime.datetime.now(moscow) + datetime.timedelta(days=2)
        with weather_for_day_api_mock(
                dt=dt,
                temperature=[2, 4],
                condition='снова ветренно',
                location=spb,
        ):
            msg = 'Послезавтра в Санкт-Петербурге от +2 до +4, снова ветренно.'
            assert f('а послезавтра') == msg

    @freeze_time('2016-10-26 21:13')
    def test_weather_from_push_with_geoid_ignore_attention(self, c):
        blocks = [{
            'type': 'attention',
            'attention_type': 'geo_changed',
            'data': None,
        }]

        spb = _city_cases('Санкт-Петербурге')
        with current_weather_api_mock(temperature=-1, condition='снег', location=spb, blocks=blocks):
            assert c(
                'update_form',
                callback_args={
                    'form_update': {
                        'name': 'personal_assistant.scenarios.get_weather',
                        'slots': [
                            {
                                'name': 'where',
                                'type': 'geo_id',
                                'value': '123'
                            }
                        ]
                    }
                }
            )['voice_text'] == 'Сейчас в Санкт-Петербурге -1, снег.'

    @freeze_time('2016-10-22 21:13')
    def test_day_range(self, f):
        dt1 = datetime.datetime.now(pytz.UTC)
        dt2 = dt1 + datetime.timedelta(days=1)
        spb = _city_cases('Санкт-Петербурге')
        with weather_for_range_api_mock(
                dates=[dt1, dt2],
                temperatures=[[1, 2], [2, 4]],
                conditions=('ясно', 'ветрено'),
                location=spb,
        ):
            msg = ('В Санкт-Петербурге сегодня от +1 до +2, ясно.\n'
                   'Завтра от +2 до +4, ветрено.')
            assert f('Погода в выходныe') == msg
            assert f('а сегодня завтра') == msg

    @freeze_time('2016-10-26 21:13')
    def test_day_part(self, f):
        now = datetime.datetime.now(moscow)
        with current_weather_api_mock(temperature=-1, condition='морозно'):
            assert f('погода какая') == 'Сейчас в Москве -1, морозно.'

        with weather_for_day_api_mock(now, temperature=[-15, -10], condition='дубак'):
            assert f('а ночью') == 'Сегодня ночью в Москве от -10 до -15, дубак.'

        with weather_for_day_api_mock(
                now + datetime.timedelta(days=1),
                temperature=[-13, -10],
                condition='дубак',
        ):
            assert f('а завтра') == 'Завтра ночью в Москве от -10 до -13, дубак.'

        with weather_for_day_api_mock(
                now + datetime.timedelta(days=1),
                temperature=[-13, -4],
                condition='лягушки падают с неба',
        ):
            assert f('а на весь день') == 'Завтра в Москве от -4 до -13, лягушки падают с неба.'

    @freeze_time('2016-10-26 20:13')
    def test_timezone(self, f):
        dt = utcnow()
        magadan = _city_cases('Магадане')
        magadan_tz = parse_tz('Asia/Magadan')
        with weather_for_day_api_mock(dt=dt.astimezone(magadan_tz) + datetime.timedelta(days=2),
                                      temperature=[-30, -20], condition='чудовищно',
                                      tz=magadan_tz.zone,
                                      location=magadan):
            assert f('какая будет погода послезавтра в магадане?') == (
                'Послезавтра в Магадане от -20 до -30, чудовищно.')

        seattle = _city_cases('Сиэтле')
        seattle_tz = parse_tz('America/Los_Angeles')
        with weather_for_day_api_mock(
                dt=dt.astimezone(seattle_tz) + datetime.timedelta(days=2),
                temperature=[10, 12],
                condition='более-менее',
                tz=seattle_tz.zone,
                location=seattle
        ):
            assert f('какая будет погода через 2 дня в Сиэтле?') == (
                'Послезавтра в Сиэтле от +10 до +12, более-менее.')

    def test_weather_fail(self, f):
        with form_handling_fail_mock():
            assert f('какая там погода') == (
                'Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.')

    def test_no_weather_for_date(self, s, handle_meta, mocker):
        blocks = [{
            'data': None,
            'error': {
                'msg': 'no weather found for the given period',
                'type': 'noweather',
            },
            'type': 'error',
        }]
        nalchick = _city_cases('Нальчике')
        with current_weather_api_mock(
                None,
                None,
                location=nalchick,
                blocks=blocks,
        ):
            result = s('скажи погоду в нальчике на вчера')
            assert result['voice_text'] == 'Нет данных о погоде в Нальчике на это число.'
            assert handle_meta(result['meta']) == [
                {
                    'type': 'error',
                    'error_type': 'noweather'
                },
            ]

    def test_no_weather_for_location(self, handle_meta, s):
        blocks = [{
            'data': None,
            'error': {
                'msg': "",
                'type': 'nogeo',
            },
            'type': 'error',
        }]
        with current_weather_api_mock(None, None, blocks=blocks):
            result = s('скажи погоду в аду')
            assert result['voice_text'] == 'К сожалению, я не знаю, где это "в аду".'
            assert handle_meta(result['meta']) == [
                {
                    'type': 'error',
                    'error_type': 'nogeo'
                },
            ]

    def test_no_weather_when_nogeo_error_but_no_where_slot(self, handle_meta, s):
        blocks = [{
            'data': None,
            'error': {
                'msg': "",
                'type': 'nogeo',
            },
            'type': 'error',
        }]
        with current_weather_api_mock(None, None, blocks=blocks):
            result = s('скажи погоду')
            assert result['voice_text'] == 'Чтобы ответить на этот вопрос мне нужно знать ваше местоположение. Но мне не удалось его определить.'  # noqa
            assert handle_meta(result['meta']) == [
                {
                    'type': 'error',
                    'error_type': 'nogeo',
                },
            ]

    @freeze_time('2017-01-26 22:44:00')
    def test_midnight_in_krasnoyarsk(self, f):
        tz = 'Asia/Krasnoyarsk'
        date = datetime.datetime.now(parse_tz(tz))
        krasnoyarsk = _city_cases('Красноярске')
        with weather_for_day_api_mock(
                date,
                temperature=[-15, -10],
                condition='дубак',
                location=krasnoyarsk,
                tz=tz,
        ):
            assert f('Погода в красноярске сегодня') == 'Сегодня в Красноярске от -10 до -15, дубак.'

    def test_geochanged_default(self, handle_meta, s):
        blocks = [{
            'type': 'attention',
            'attention_type': 'geo_changed',
            'data': None,
        }]

        with current_weather_api_mock(temperature=-1, condition='снег', blocks=blocks):
            result = s('расскажи погоду')
            assert result['voice_text'] == (
                'Первое правило погоды рядом с вами — никому не рассказывать о погоде рядом с вами. А теперь о погоде в Москве. Сейчас в Москве -1, снег.'  # noqa
            )
            assert handle_meta(result['meta']) == [
                {
                    'type': 'attention',
                    'attention_type': 'geo_changed'
                },
            ]


class TestWhatCanYouDo:
    SUGGEST_SETS_NAMES = {
        0: ['onboarding__what_can_you_do', 'onboarding__get_weather__now',
            'onboarding__search_internet__open_vk', 'onboarding__find_poi__pharmacy_nearby',
            'onboarding__find_poi__barbershop', 'onboarding__search_internet__kozlovsky_height',
            'onboarding__convert__dollar_rate_today', 'onboarding__handcrafted__what_is_your_name'],
        1: ['onboarding__what_can_you_do', 'onboarding__get_weather__tomorrow',
            'onboarding__search_internet__open_odnoklassniki', 'onboarding__find_poi__cafe_nearby',
            'onboarding__find_poi__carwash_nearby', 'onboarding__search_internet__pushkin_dates',
            'onboarding__convert__euro_rate_today', 'onboarding__handcrafted__tell_me_a_tale'],
        2: ['onboarding__what_can_you_do', 'onboarding__get_weather__weekend',
            'onboarding__search_internet__open_facebook', 'onboarding__find_poi__restaurant_nearby',
            'onboarding__find_poi__cinema', 'onboarding__search_internet__summer_duration',
            'onboarding__convert__hundred_dollar_in_roubles', 'onboarding__handcrafted__life_on_mars'],
        3: ['onboarding__what_can_you_do', 'onboarding__get_weather__couple_days',
            'onboarding__search_internet__yamaps', 'onboarding__find_poi__dinner',
            'onboarding__find_poi__pharmacy_nearby',
            'onboarding__search_internet__termodynamics_second_law',
            'onboarding__convert__hundred_euros_in_roubles',
            'onboarding__handcrafted__who_are_your_friends'],
        4: ['onboarding__what_can_you_do', 'onboarding__get_weather__today',
            'onboarding__search_internet__yanews', 'onboarding__find_poi__coffee_nearby',
            'onboarding__find_poi__flowershop_nearby', 'onboarding__search_internet__robotlaws_whatis',
            'onboarding__convert__euro_price_today', 'onboarding__handcrafted__what_is_love']
    }

    def test_answer(self, f):
        with suggests_mock(self.SUGGEST_SETS_NAMES[0], form_name='personal_assistant.scenarios.what_can_you_do'):
            assert f('что ты умеешь') == 'Я отвечаю на любые вопросы или в поиск отправляю запросы. Строю маршруты с точностью до минуты. Предсказываю погоду с точностью до времени года. Шутка. В общем, попробуйте сами, давайте пообщаемся с вами.'  # noqa

    @pytest.mark.parametrize('suggests_set_num', [0, 1, 2, 3, 4])
    def test_suggests(self, s, suggests_set_num):
        suggest_names = self.SUGGEST_SETS_NAMES[suggests_set_num]
        with suggests_mock(suggest_names, form_name='personal_assistant.scenarios.what_can_you_do'):
            res = s('что ты можешь')
            assert res['voice_text']
            assert not res['directives']

            assert len(res['suggests']) == len(suggest_names) + 2  # +2 because of feedback__positive/negative suggests.

            for vins_suggest, correct_suggest_name in zip(res['suggests'][2:], suggest_names):
                assert vins_suggest['directives'][-1]['payload']['suggest_block']['suggest_type'] == correct_suggest_name  # noqa
                assert vins_suggest['directives'][-1]['payload']['utterance']


class TestSessionStart:
    def test_session_start(self, c):
        suggest_names = TestWhatCanYouDo.SUGGEST_SETS_NAMES[0]
        with suggests_mock(suggest_names):
            res = c('on_reset_session', mode='greeting')
            assert not res['directives']
            assert len(res['suggests']) == len(suggest_names)
            assert res['should_listen'] is False
            assert res['force_voice_answer'] is False

            for vins_suggest, correct_suggest_name in zip(res['suggests'], suggest_names):
                assert vins_suggest['directives'][-1]['payload']['suggest_block']['suggest_type'] == correct_suggest_name  # noqa
                assert vins_suggest['directives'][-1]['payload']['utterance']

    def test_session_start_messages(self, c):
        with suggests_mock([]):
            # No message for old clients without mode
            res = c('on_reset_session')
            assert res['voice_text'] is None

            # Phrase on greeting
            res = c('on_reset_session', callback_args={'mode': 'greeting'})
            assert res['voice_text'] == 'Чем могу помочь?'

            # Phrase on onboarding
            res = c('on_reset_session', callback_args={'mode': 'clear_history'})
            assert res['voice_text'] == 'Чем я могу быть полезна?'

            # No phrase on unknown modes
            res = c('on_reset_session', callback_args={'mode': 'some_unknown_mode'})
            assert res['voice_text'] is None


class TestGetGreetings:
    def test_get_greetings(self, c):
        with cards_mock(
            cards=[
                {
                    "type": "div_card",
                    "data": {
                        "store_url": "https://alice.yandex.ru/help",
                        "cases": [
                            {
                                "description": "Про зайца, про репку. В общем, про что хотите.",
                                "idx": "onboarding__music_fairy_tale2",
                                "activation": "Расскажи сказку",
                                "recommendation_type": "static",
                                "recommendation_source": "service",
                                "logo": "logo_url"
                            },
                            {
                                "description": "Дайте послушать, а я назову исполнителя и песню.",
                                "idx": "onboarding__music_what_is_playing2",
                                "activation": "Что за музыка играет?",
                                "recommendation_type": "static",
                                "recommendation_source": "service",
                                "logo": "logo_url"
                            },
                            {
                                "description": "Найду лучший маршрут из А в Б.",
                                "idx": "onboarding__show_route",
                                "activation": "Как добраться до работы?",
                                "recommendation_type": "static",
                                "recommendation_source": "service",
                                "logo": "logo_url"
                            }
                        ]
                    },
                    "card_template": "skill_recommendation"
                }
            ],
            slots_map={'card_name': 'get_greetings'},
            form_name="personal_assistant.scenarios.skill_recommendation"
        ):
            response = c("on_get_greetings")
            assert len(response['cards']) == 1
            assert response['cards'][0]['type'] == 'div_card'
            assert len(response['cards'][0]['body']['states'][0]['blocks']) == 5

    def test_get_greetings_with_cloud_ui(self, c):
        with cards_mock(
            cards=[
                {
                    "card_template": "skill_recommendation",
                    "data": {
                        "cases": [
                            {
                                "activation": "Расскажи сказку",
                                "description": "У меня есть много сказок. Про кого хотите послушать?",
                                "idx": "onboarding_music_fairy_tale2",
                                "logo": "https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-x2",
                                "logo_amelie_bg_url": "https://avatars.mds.yandex.net/get-dialogs/1017510/tallLogo10/logo-bg-image-tall-x2",
                                "logo_bg_color": "#919cb5",
                                "look": "internal",
                                "name": "Сказки",
                                "recommendation_source": "get_greetings",
                                "recommendation_type": "editorial#"
                            },
                            {
                                "activation": "Сколько ехать до дома?",
                                "description": "Найду лучший маршрут из А в Б.",
                                "idx": "onboarding_show_route",
                                "logo": "https://avatars.mds.yandex.net/get-dialogs/1017510/onboard_Route/mobile-logo-x2",
                                "logo_amelie_bg_url": "https://avatars.mds.yandex.net/get-dialogs/1525540/navi/logo-bg-image-tall-x2",
                                "logo_bg_color": "#919cb5",
                                "look": "internal",
                                "name": "Построй маршрут",
                                "recommendation_source": "get_greetings",
                                "recommendation_type": "editorial#"
                            },
                            {
                                "activation": "запусти навык Игра прятки",
                                "description": "🏆Игра прятки - победитель премии Алисы!",
                                "idx": "a93a1c1c-5fd2-4c00-9bf9-4d25cc1917dd",
                                "logo": "https://avatars.mds.yandex.net/get-dialogs/1027858/ee124dce95b341e28c00/mobile-logo-x2",
                                "logo_amelie_bg_url": "https://avatars.mds.yandex.net/get-dialogs/1530877/logo-bg-image-tallw/logo-bg-image-tall-x2",
                                "logo_amelie_bg_wide_url": "https://avatars.mds.yandex.net/get-dialogs/1530877/logo-bg-image-tallw/logo-bg-image-tallw-x2",
                                "logo_amelie_fg_url": "https://avatars.mds.yandex.net/get-dialogs/1027858/ee124dce95b341e28c00/logo-icon40x40_x2",
                                "logo_bg_color": "#919cb5",
                                "look": "external",
                                "name": "Игра прятки",
                                "recommendation_source": "get_greetings",
                                "recommendation_type": "editorial#"
                            }
                        ],
                        "store_url": "https://dialogs.yandex.ru/store/essentials?utm_source=Yandex_Alisa&utm_medium=onboarding"
                    },
                    "type": "div_card"
                },
                {
                    "form_update": {
                        "name": "personal_assistant.scenarios.onboarding",
                        "resubmit": True
                    },
                    "suggest_type": "onboarding__next",
                    "type": "suggest"
                },
                {
                    "data": {
                        "action_id": "ac8670bf-5ea61b1a-cb02f7a-795132b3",
                        "frame_action": {
                            "directives": {
                                "list": [
                                    {
                                        "type_text_directive": {
                                            "text": "Расскажи сказку"
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "external_source_action",
                                            "payload": {
                                                "utm_campaign": "",
                                                "utm_content": "textlink",
                                                "utm_medium": "get_greetings",
                                                "utm_source": "Yandex_Alisa",
                                                "utm_term": ""
                                            }
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "on_card_action",
                                            "payload": {
                                                "card_id": "skill_recommendation",
                                                "case_name": "skill_recommendation__get_greetings__editorial#__onboarding_music_fairy_tale2",
                                                "intent_name": "personal_assistant.scenarios.skill_recommendation",
                                                "item_number": "1"
                                            }
                                        }
                                    }
                                ]
                            }
                        }
                    },
                    "type": "frame_action"
                },
                {
                    "data": {
                        "action_id": "cbcf1bde-2ac91d44-db2112ca-c9da955f",
                        "frame_action": {
                            "directives": {
                                "list": [
                                    {
                                        "type_text_directive": {
                                            "text": "Сколько ехать до дома?"
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "external_source_action",
                                            "payload": {
                                                "utm_campaign": "",
                                                "utm_content": "textlink",
                                                "utm_medium": "get_greetings",
                                                "utm_source": "Yandex_Alisa",
                                                "utm_term": ""
                                            }
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "on_card_action",
                                            "payload": {
                                                "card_id": "skill_recommendation",
                                                "case_name": "skill_recommendation__get_greetings__editorial#__onboarding_show_route",
                                                "intent_name": "personal_assistant.scenarios.skill_recommendation",
                                                "item_number": "2"
                                            }
                                        }
                                    }
                                ]
                            }
                        }
                    },
                    "type": "frame_action"
                },
                {
                    "data": {
                        "action_id": "af8d7617-6e791d14-42cdb6ba-19e58fee",
                        "frame_action": {
                            "directives": {
                                "list": [
                                    {
                                        "type_text_directive": {
                                            "text": "запусти навык Игра прятки"
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "external_source_action",
                                            "payload": {
                                                "utm_campaign": "",
                                                "utm_content": "textlink",
                                                "utm_medium": "get_greetings",
                                                "utm_source": "Yandex_Alisa",
                                                "utm_term": ""
                                            }
                                        }
                                    },
                                    {
                                        "callback_directive": {
                                            "ignore_answer": True,
                                            "name": "on_card_action",
                                            "payload": {
                                                "card_id": "skill_recommendation",
                                                "case_name": "skill_recommendation__get_greetings__editorial#__a93a1c1c-5fd2-4c00-9bf9-4d25cc1917dd",
                                                "intent_name": "personal_assistant.scenarios.skill_recommendation",
                                                "item_number": "3"
                                            }
                                        }
                                    }
                                ]
                            }
                        }
                    },
                    "type": "frame_action"
                },
                {
                    "command_sub_type": "show_buttons",
                    "command_type": "show_buttons",
                    "data": {
                        "buttons": [
                            {
                                "action_id": "ac8670bf-5ea61b1a-cb02f7a-795132b3",
                                "text": "У меня есть много сказок. Про кого хотите послушать?",
                                "theme": {
                                    "image_url": "https://avatars.mds.yandex.net/get-dialogs/1525540/onboard_FairyTale/mobile-logo-x2"
                                },
                                "title": "Сказки"
                            },
                            {
                                "action_id": "cbcf1bde-2ac91d44-db2112ca-c9da955f",
                                "text": "Найду лучший маршрут из А в Б.",
                                "theme": {
                                    "image_url": "https://avatars.mds.yandex.net/get-dialogs/1017510/onboard_Route/mobile-logo-x2"
                                },
                                "title": "Построй маршрут"
                            },
                            {
                                "action_id": "af8d7617-6e791d14-42cdb6ba-19e58fee",
                                "text": "🏆Игра прятки - победитель премии Алисы!",
                                "theme": {
                                    "image_url": "https://avatars.mds.yandex.net/get-dialogs/1027858/ee124dce95b341e28c00/mobile-logo-x2"
                                },
                                "title": "Игра прятки"
                            }
                        ],
                        "screen_id": "cloud_ui"
                    },
                    "type": "command"
                },
                {
                    "data": {
                        "features": {
                            "builtin_feedback": {
                                "enabled": True
                            }
                        }
                    },
                    "type": "client_features"
                },
                {
                    "data": "EjFwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnNraWxsX3JlY29tbWVuZGF0aW9uSgpvbmJvYXJkaW5n",
                    "type": "scenario_analytics_info"
                }
            ],
            slots_map={'card_name': 'get_greetings'},
            form_name="personal_assistant.scenarios.skill_recommendation"
        ):
            response = c("on_get_greetings")

            # check card
            assert len(response['cards']) == 1
            assert response['cards'][0]['type'] == 'div_card'
            assert len(response['cards'][0]['body']['states'][0]['blocks']) == 5

            # check frame actions
            actions_ids = list(response['frame_actions'])
            assert len(actions_ids) == 3

            # check directive
            assert len(response['directives']) == 1
            directive = response['directives'][0]
            assert directive['name'] == 'show_buttons'
            assert directive['payload']['screen_id'] == 'cloud_ui'
            assert len(directive['payload']['buttons']) == 3
            for button in directive['payload']['buttons']:
                assert button['title']
                assert button['text']


class TestHelpButtonClick:
    def test_help_button_click(self, c):
        with cards_mock(
            cards=[
                {
                    'card_template': 'onboarding',
                    'data': {
                        'cases': ['onboarding__alice_songs', 'onboarding__open_application2',
                                  'onboarding__music_fairy_tale', 'onboarding__weather', 'onboarding__alarm'],

                        'icons': [],
                    },
                    'type': 'div_card'
                }
            ],
            slots_map={'mode': 'help', 'set_number': 7}
        ):
            response = c('on_reset_session', callback_args={'mode': 'help'})
            assert len(response['cards']) == 1
            assert response['cards'][0]['type'] == 'div_card'


class TestSearch:
    def test_command(self, f):
        with search_api_mock(query='рецепты салатов'):
            assert f('рецепты салатов') == 'Найдётся всё!'

    def test_action_no_query(self, f):
        assert f('найди в яндексе') == 'Что для вас найти?'
        assert f('покажи источник') == 'Что вы хотите найти?'

    def test_suggest_factoid(self, s):
        with search_api_mock(query='рецепты салатов', factoid='Чука — лучший салат.'):
            answer = s('рецепты салатов')
            assert answer['voice_text'] == 'Есть такой ответ'
            assert answer['cards'][0]['text'] == 'Чука — лучший салат.'
            assert answer['cards'][0]['buttons'] == [
                {
                    'title': 'Поискать в Яндексе',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'open_uri',
                            'sub_name': 'render_buttons_open_uri',
                            'payload': {
                                'uri': 'https://yandex.ru/search/?text=рецепты салатов'
                            }
                        },
                        _suggest_logging_action(
                            caption='Поискать в Яндексе',
                            suggest_type='search__serp',
                            uri='https://yandex.ru/search/?text=рецепты салатов'
                        ),
                    ]
                }
            ]

    def test_suggest_no_factoid(self, handle_response, s):
        with search_api_mock(query='рецепты салатов'):
            assert handle_response(s('рецепты салатов')) == {
                'meta': [],
                'voice_text': 'Найдётся всё!',
                'cards': [
                    {
                        'type': 'text_with_button',
                        'text': 'Найдётся всё!',
                        'buttons': [
                            {
                                'title': 'Поискать в Яндексе',
                                'type': 'action',
                                'directives': [
                                    {
                                        'type': 'client_action',
                                        'name': 'open_uri',
                                        'sub_name': 'render_buttons_open_uri',
                                        'payload': {
                                            'uri': 'https://yandex.ru/search/?text=рецепты салатов'
                                        }
                                    },
                                    _suggest_logging_action(
                                        caption='Поискать в Яндексе',
                                        suggest_type='search__serp',
                                        uri='https://yandex.ru/search/?text=рецепты салатов'
                                    ),
                                ]
                            }
                        ],
                    }
                ],
                'directives': [
                    {
                        'name': 'open_uri',
                        'sub_name': 'personal_assistant.scenarios.search',
                        'payload': {
                            'uri': 'https://yandex.ru/search/?text=рецепты салатов'
                        },
                        'type': 'client_action'
                    }
                ],
                'suggests': [
                    _feedback_suggest(is_positive=True),
                    _feedback_suggest(is_positive=False),
                ],
                'should_listen': False,
                'force_voice_answer': False,
                'autoaction_delay_ms': None,
                'special_buttons': [],
                'features': {
                    'form_info': {
                        'intent': 'personal_assistant.scenarios.search',
                        'is_continuing': False,
                        'expects_request': False,
                    }
                },
            }

    def test_factoid(self, fs):
        with search_api_mock(query='высота эвереста', factoid='8848 м', factoid_tts='8848 метров'):
            assert fs('высота эвереста') == ('8848 метров', '8848 м')

    def test_factoid_with_tag(self, fs):
        with search_api_mock(query='высота эвереста', factoid='8848 м', factoid_tts='8848 #meter'):
            assert fs('высота эвереста') == ('8848 метров', '8848 м')

    def test_factoid_no_tts(self, fs):
        with search_api_mock(query='высота эвереста', factoid='8848 м', factoid_tts=None):
            assert fs('высота эвереста') == ('Есть такой ответ', '8848 м')

    def test_strip_activation_processor(self, f, mock_request):
        with form_handling_mock({'form': {'name': 'personal_assistant.scenarios.search', 'slots': []}, 'blocks': []}):
            f('Алиса, кто такой навальный')
            assert mock_request.last_request.json()['form']['slots'][0]['value'] == 'кто такой навальный'

            f('кто такой путин, Алиса')
            assert mock_request.last_request.json()['form']['slots'][0]['value'] == 'кто такой путин'

    def test_suggest_with_one_type(self, s):
        blocks = [
            {
                'data': {'query': 'мелания трамп'},
                'suggest_type': 'search__see_also',
                'type': 'suggest'
            }, {
                'data': {'query': 'иванка мари трамп'},
                'suggest_type': 'search__see_also',
                'type': 'suggest'
            }, {
                'data': {'query': 'трамп пам памп'},
                'suggest_type': 'search__see_also',
                'type': 'suggest'
            }
        ]

        def gen_search_button(text):
            return {
                'title': text,
                'type': 'action',
                'directives': [
                    {
                        'type': 'client_action',
                        'name': 'type',
                        'sub_name': 'render_buttons_type',
                        'payload': {'text': text},
                    },
                    _suggest_logging_action(
                        caption=text,
                        suggest_type='search__see_also',
                        utterance=text,
                        block_data={'query': text},
                    ),
                ],
            }

        with search_api_mock(query='кто такой трамп', blocks=blocks):
            answer = s('кто такой трамп')
            assert answer['suggests'] == [
                _feedback_suggest(is_positive=True),
                _feedback_suggest(is_positive=False),
                gen_search_button('мелания трамп'),
                gen_search_button('иванка мари трамп'),
                gen_search_button('трамп пам памп'),
            ]


class TestShowTraffic:
    loc = _city_cases('Москве')
    info = {'hint': 'Адище', 'level': '10', 'url': 'https://yandex.ru/maps/213/moscow/probki/'}

    def test_no_traffic_from_suggest(self, handle_event):
        blocks = [{
            'data': None,
            'error': {
                'msg': '',
                'type': 'notraffic',
            },
            'type': 'error',
        }]
        with traffic_api_mock(self.loc, self.info, blocks=blocks):
            assert handle_event(SuggestedInputEvent('пробки в москве'))['voice_text'] == (
                'К сожалению, я не могу ответить на вопрос о дорожной ситуации в этом месте.'
            )


class TestShowRoute:
    def test_command(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4'):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 10 минут на (машине|авто), 20 минут на транспорте или 15 минут пешком. Это путь до адреса Ружейный переулок 4.',  # noqa
                f('построй маршрут до ружейного 4')
            )

    def test_pedestrian_route_command(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4'):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 15 минут. Это путь до адреса Ружейный переулок 4.',
                f('построй пешеходный маршрут до ружейного 4')
            )

    def test_car_route_command(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Тимура Фрунзе', house_to='16',
                                 what_to='тимура фрунзе 16'):
            assert f('сколько ехать на машине до тимура фрунзе 16') == (
                '10 минут с учетом пробок. Это путь до адреса Тимура Фрунзе 16.'
            )

    def test_public_transport_route_command(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4',
                                 transfers=1):
            assert f('сколько ехать на транспорте до ружейного 4') == (
                '20 минут, включая 1 пересадку и 1 километр пешком. Это путь до адреса Ружейный переулок 4.'
            )

    def test_public_transport_route_command_no_transfers(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4'):
            assert f('сколько ехать на транспорте до ружейного 4') == (
                '20 минут, включая 1 километр пешком. Это путь до адреса Ружейный переулок 4.'
            )

    def test_vehicle_route_command(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4'):
            assert f('сколько ехать до ружейного 4') == (
                '10 минут на машине или 20 минут на транспорте. Это путь до адреса Ружейный переулок 4.'
            )

    def _assert_show_route_on_map(self, answer, type=''):
        assert re.match('(?:Сейчас открою|Открываю) маршрут на карте.', answer['voice_text'])
        assert answer['directives'] == [
            {
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'personal_assistant.scenarios.show_route__show_route_on_map',
                'payload': {
                    'uri': 'http://maps.ru/' + type
                }
            }
        ]

    def test_suggests(self, s):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4'):
            answer = s('сколько идти до ружейного 4')
            assert answer['voice_text'] == (
                '15 минут. Это путь до адреса Ружейный переулок 4.')
            assert answer['suggests'] == [
                _feedback_suggest(is_positive=True),
                _feedback_suggest(is_positive=False),
                {
                    'title': 'На авто',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'type',
                            'sub_name': 'render_buttons_type',
                            'payload': {
                                'text': 'А на авто?',
                            }
                        },
                        _suggest_logging_action(
                            caption='На авто',
                            suggest_type='show_route__go_by_car',
                            utterance='А на авто?',
                        ),
                    ]
                },
                {
                    'title': 'На транспорте',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'type',
                            'sub_name': 'render_buttons_type',
                            'payload': {
                                'text': 'А если на транспорте?',
                            }
                        },
                        _suggest_logging_action(
                            caption='На транспорте',
                            suggest_type='show_route__go_by_public_transport',
                            utterance='А если на транспорте?',
                        ),
                    ]
                },
                {
                    'title': 'Пешком',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'type',
                            'sub_name': 'render_buttons_type',
                            'payload': {
                                'text': 'А если пешком?',
                            }
                        },
                        _suggest_logging_action(
                            caption='Пешком',
                            suggest_type='show_route__go_by_foot',
                            utterance='А если пешком?',
                        ),
                    ]
                },
            ]
            assert answer['cards'] == [{
                'text': '15 минут. Это путь до адреса Ружейный переулок 4.',
                'type': 'text_with_button',
                'buttons': [
                    {
                        'title': 'Маршрут на карте',
                        'type': 'action',
                        'directives': [
                            {
                                'type': 'client_action',
                                'name': 'open_uri',
                                'sub_name': 'render_buttons_open_uri',
                                'payload': {
                                    'uri': 'http://maps.ru/pedestrian',
                                }
                            },
                            _suggest_logging_action(
                                caption='Маршрут на карте',
                                suggest_type='show_route__show_on_map',
                                uri='http://maps.ru/pedestrian',
                            ),
                        ]
                    }
                ]
            }]
            answer = s('покажи маршрут на транспорте')
            self._assert_show_route_on_map(answer, 'public_transport')
            answer = s('покажи автомобильный маршрут')
            self._assert_show_route_on_map(answer, 'car')
            answer = s('покажи на карте')
            self._assert_show_route_on_map(answer, 'car')

    def test_no_route(self, s):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Тимура Фрунзе', house_to='16',
                                 what_to='тимура фрунзе 16',
                                 time_by_car=None, time_by_public_transport=None, time_on_foot=None):
            assert s('сколько ехать на машине до тимура фрунзе 16')['voice_text'] == (
                'К сожалению, я не смогла построить маршрут на авто до адреса Тимура Фрунзе 16. Давайте ещё разок.'
            )
            answer = s('покажи на карте')
            assert answer['voice_text'] == 'Я не смогла построить маршрут, но сейчас открою карту.'
            assert answer['directives'] == [
                {
                    'type': 'client_action',
                    'name': 'open_uri',
                    'sub_name': 'personal_assistant.scenarios.show_route__show_route_on_map',
                    'payload': {
                        'uri': 'http://maps.ru/fallback'
                    }
                }
            ]

    def test_command_only_pedestrian_available(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4',
                                 time_by_car=None, time_by_public_transport=None):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 15 минут пешком. Это путь до адреса Ружейный переулок 4.',
                f('построй маршрут до ружейного 4')
            )

    def test_command_only_car_available(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4',
                                 time_by_public_transport=None, time_on_foot=None):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 10 минут на (машине|авто). Это путь до адреса Ружейный переулок 4.',
                f('построй маршрут до ружейного 4')
            )

    def test_command_only_public_transport_available(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4',
                                 time_by_car=None, transfers=1, time_on_foot=None):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 20 минут на транспорте. Это путь до адреса Ружейный переулок 4.',
                f('построй маршрут до ружейного 4')
            )

    def test_open_route_on_map_not_every_root_is_available(self, f):
        with show_route_api_mock(street_from='Льва Толстого', house_from='16',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_to='ружейного 4',
                                 time_by_car=None, transfers=1, time_on_foot=None):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 20 минут на транспорте. Это путь до адреса Ружейный переулок 4.',
                f('построй маршрут до ружейного 4')
            )
            assert f('покажи маршрут на карте') == 'Сейчас открою маршрут на карте.'

    def test_formfilling(self, f):
        with show_route_api_mock(street_from='улица Серафимовича', house_from='2',
                                 what_from='серафимовича 2', ask_where_to=True):
            assert f('как доехать от серафимовича 2') == 'Куда нужно добраться?'
        with show_route_api_mock(street_from='улица Серафимовича', house_from='2',
                                 street_to='Ружейный переулок', house_to='4',
                                 what_from='серафимовича 2', what_to='ружейного 4'):
            assert re.match(
                '(Путь|Маршрут|Дорога) займет 10 минут на машине или 20 минут на транспорте. Это путь от адреса улица Серафимовича 2 до адреса Ружейный переулок 4.',  # noqa
                f('до ружейного 4')
            )
            assert f('а сколько идти?') == (
                '15 минут. Это путь от адреса улица Серафимовича 2 до адреса Ружейный переулок 4.'
            )
            assert re.match(
                '10 минут с учетом пробок. Это путь от адреса улица Серафимовича 2 до адреса Ружейный переулок 4',
                f('а напомни сколько на машине')
            )
        with show_route_api_mock(street_from='улица Серафимовича', house_from='2',
                                 street_to='Тимура Фрунзе', house_to='20',
                                 what_from='серафимовича 2', what_to='тимура фрунзе 20'):
            assert re.match(
                '10 минут с учетом пробок. Это путь от адреса улица Серафимовича 2 до адреса Тимура Фрунзе 20.',
                f('а до тимура фрунзе 20?')
            )


class TestFindPoiInfo:
    def test_command_no_address(self, f):
        with poi_search_api_mock(make_where_required=True):
            assert f('найди адрес') == 'Пожалуйста, уточните адрес.'
        with poi_search_api_mock(make_what_required=True):
            assert f('позвони') == 'В моей базе нет номера этой организации.'

    def test_command_address(self, f):
        with poi_search_api_mock(name='Розетки & выключатели', street='1-я Фрунзенская улица', house='31',
                                 what='магазина розеток и выключателей', where='nearest'):
            assert f('найди адрес ближайшего магазина розеток и выключателей') == (
                'Вам может подойти "Розетки & выключатели", 1-я Фрунзенская улица 31.')
        with geo_search_api_mock(street='Столешников переулок', where='столешников переулок'):
            assert f('столешников переулок') == 'Столешников переулок — могу открыть карту для этого адреса.'

    def test_opening_hours(self, f):
        with poi_search_api_mock(name='Розетки & выключатели', street='1-я Фрунзенская улица', house='31',
                                 hours=poi_opening_hours_round_the_clock(),
                                 what='магазина розеток и выключателей', where='nearest'):
            assert f('найди адрес ближайшего магазина розеток и выключателей') == (
                'Вам может подойти "Розетки & выключатели", 1-я Фрунзенская улица 31. Работает круглосуточно.')
        with poi_search_api_mock(name='Супер бассейн', street='Водная улица', house='13',
                                 hours=poi_opening_hours(opening_time='9:00', closing_time='18:00', is_open=True),
                                 what='бассейна', where='nearest'):
            assert f('а бассейна?') == (
                'По адресу Водная улица 13 есть "Супер бассейн". Работает с 9 утра до 6 вечера. Сейчас открыто.')
        with poi_search_api_mock(name='Супер бассейн', street='Водная улица', house='13',
                                 hours=poi_opening_hours_list([('9:00', '12:00'), ('14:00', '18:00')], is_open=True),
                                 what='бассейна', where='nearest'):
            assert f('а бассейна?') == (
                'Я знаю, что по адресу Водная улица 13 есть "Супер бассейн". Работает с 9 утра до 12 часов дня и с 2 часов дня до 6 вечера. Сейчас открыто.')  # noqa
        with poi_search_api_mock(name='Супер бассейн', street='Водная улица', house='13',
                                 hours=poi_opening_hours_list([('9:00', '12:00'), ('14:00', '18:00'), ('19:00', '23:00')], is_open=True),  # noqa
                                 what='бассейна', where='nearest'):
            assert f('а бассейна?') == (
                'По адресу Водная улица 13 есть "Супер бассейн". Работает с 9 утра до 12 часов дня, с 2 часов дня до 6 вечера и с 7 вечера до 11 вечера. Сейчас открыто.')  # noqa
        with poi_search_api_mock(name='Рюмочная номер 1', street='1-я Питейная улица', house='22',
                                 hours=poi_opening_hours(opening_time='9:00:00',
                                                         closing_time='23:00:00',
                                                         is_open=False),
                                 what='рюмочной'):
            assert f('найди адрес рюмочной') == (
                'Вот что нашлось: "Рюмочная номер 1", 1-я Питейная улица 22. Работает с 9 утра до 11 вечера. Сейчас закрыто.')  # noqa

    def test_ask_for_geo(self, f):
        with poi_search_api_mock(what='кинотеатр', make_where_required=True):
            assert f('найди кинотеатр') == 'Пожалуйста, уточните адрес.'

    def test_command_address_with_location(self, f):
        with poi_search_api_mock(name='Икея', street='Какая-то улица в Химках', house='13',
                                 what='икеи', where='химках'):
            assert f('найди адрес икеи которая в химках') == (
                'Вам может подойти "Икея", Какая-то улица в Химках 13.')

    def test_formfiller_address(self, f):
        with poi_search_api_mock(make_where_required=True):
            assert f('найди мне адрес') == 'Пожалуйста, уточните адрес.'
        with poi_search_api_mock(name='Розетки & выключатели', street='1-я Фрунзенская улица', house='31',
                                 what='магазин розеток и выключателей'):
            assert f('магазин розеток и выключателей') == (
                'Я знаю, что по адресу 1-я Фрунзенская улица 31 есть "Розетки & выключатели".')
        with poi_search_api_mock(name='Аптека 36,6', street='Зубовский бульвар', house='17с1',
                                 what='аптеки', where='nearest'):
            assert f('и ближайшей аптеки') == 'Как насчет "Аптека 36,6", Зубовский бульвар 17с1?'

    def test_formfiller_address_with_location(self, f):
        with poi_search_api_mock(make_where_required=True):
            assert f('найди мне адрес') == 'Пожалуйста, уточните адрес.'
        with poi_search_api_mock(name='Икея', street='Какая-то улица где-то', house='13', what='икея'):
            assert f('икея') == 'Я знаю, что по адресу Какая-то улица где-то 13 есть "Икея".'
        with poi_search_api_mock(name='Икея', street='Какая-то улица в Химках', house='666',
                                 what='икея', where='химках'):
            assert f('мне в химках') == 'Как насчет "Икея", Какая-то улица в Химках 666?'

    def test_address_then_details(self, s):
        with poi_search_api_mock(name='Розетки & выключатели', street='1-я Фрунзенская улица', house='31',
                                 object_id='666', what='магазина розеток и выключателей', where='nearest'):
            assert s('найди адрес ближайшего магазина розеток и выключателей')['voice_text'] == (
                'Вам может подойти "Розетки & выключатели", 1-я Фрунзенская улица 31.')
            answer = s('там дорого?')
            assert answer['voice_text'] == 'Открываю карточку с подробной информацией.'
            assert answer['directives'] == [
                {
                    'type': 'client_action',
                    'name': 'open_uri',
                    'sub_name': 'personal_assistant.scenarios.find_poi__details',
                    'payload': {
                        'uri': 'https://yandex.ru/maps/org/666'
                    }
                }
            ]
            assert answer['cards'] == [
                {
                    'text': 'Открываю карточку с подробной информацией.',
                    'type': 'simple_text',
                    'tag': None,
                }
            ]

    def test_suggest(self, s):
        with poi_search_api_mock(name='Розетки & выключатели', street='1-я Фрунзенская улица', house='31',
                                 object_id='666', what='магазина розеток и выключателей', where='nearest',
                                 new_result_index=1):
            answer = s('найди адрес ближайшего магазина розеток и выключателей')
            assert answer['voice_text'] == 'Вам может подойти "Розетки & выключатели", 1-я Фрунзенская улица 31.'
            assert answer['suggests'] == [
                _feedback_suggest(is_positive=True),
                _feedback_suggest(is_positive=False),
                {
                    'title': 'Ещё вариант',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'type',
                            'sub_name': 'render_buttons_type',
                            'payload': {
                                'text': 'Ещё',
                            }
                        },
                        _suggest_logging_action(
                            caption='Ещё вариант',
                            suggest_type='find_poi__next',
                            utterance='Ещё'
                        ),
                    ]
                },
            ]
            assert answer['cards'][0]['buttons'] == [
                {
                    'title': 'Подробнее',
                    'type': 'action',
                    'directives': [
                        {
                            'type': 'client_action',
                            'name': 'open_uri',
                            'sub_name': 'render_buttons_open_uri',
                            'payload': {
                                'uri': 'https://yandex.ru/maps/org/666',
                            }
                        },
                        _suggest_logging_action(
                            caption='Подробнее',
                            suggest_type='find_poi__details',
                            uri='https://yandex.ru/maps/org/666'
                        ),
                    ]
                },
            ]

    def test_address_then_location_then_details(self, s):
        with poi_search_api_mock(name='Сбербанк', street='Брянская улица', house='8', what='отделения сбербанка'):
            assert s('адрес отделения сбербанка')['voice_text'] == (
                'Вам может подойти "Сбербанк", Брянская улица 8.')
        with poi_search_api_mock(name='Сбербанк', street='Льва Толстого', house='10', object_id='666',
                                 what='отделения сбербанка', where='льва толстого'):
            assert s('а на льва толстого')['voice_text'] == 'По адресу Льва Толстого 10 есть "Сбербанк".'
        answer = s('подробнее')
        assert answer['voice_text'] == 'Открываю подробную информацию.'
        assert answer['directives'] == [
            {
                'type': 'client_action',
                'name': 'open_uri',
                'sub_name': 'personal_assistant.scenarios.find_poi__details',
                'payload': {
                    'uri': 'https://yandex.ru/maps/org/666'
                }
            }
        ]

    def test_address_scroll(self, s):
        with poi_search_api_mock(name='Бар What the 5', street='Берсеневская набережная', house='6с3',
                                 what='бар', where='nearby'):
            assert s('найди бар поблизости')['voice_text'] == (
                'Вам может подойти "Бар What the 5", Берсеневская набережная 6с3.')
        with poi_search_api_mock(name='Кастл гриль бар', street='Комсомольский проспект', house='4а',
                                 what='бар', where='nearby', form='find_poi__scroll__next'):
            assert s('давай другой')['voice_text'] == (
                'По адресу Комсомольский проспект 4а есть "Кастл гриль бар".')
        with poi_search_api_mock(name='FF Restaurant & Bar', street='улица Тимура Фрунзе', house='11к2',
                                 object_id='666', what='бар', where='nearby', form='find_poi__scroll__next'):
            assert s('еще')['voice_text'] == (
                'Я знаю, что по адресу улица Тимура Фрунзе'
                ' 11к2 есть "FF Restaurant & Bar".'
            )
            answer = s('подробнее')
            assert answer['voice_text'] == 'Давайте узнаем всё подробно.'
            assert answer['directives'] == [
                {
                    'type': 'client_action',
                    'name': 'open_uri',
                    'sub_name': 'personal_assistant.scenarios.find_poi__details',
                    'payload': {
                        'uri': 'https://yandex.ru/maps/org/666'
                    }
                }
            ]
        with poi_search_api_mock(name='Бар в гостинице', street='улица Остоженка', house='32с0',
                                 what='бар', where='nearby', form='find_poi__scroll__next'):
            assert s('следующий')['voice_text'] == 'Вам может подойти "Бар в гостинице" по адресу улица Остоженка 32с0.'
        with poi_search_api_mock(name='FF Restaurant & Bar', street='улица Тимура Фрунзе', house='11к2',
                                 what='бар', where='nearby', form='find_poi__scroll__prev'):
            assert s('предыдущий')['voice_text'] == (
                'Как насчет "FF Restaurant & Bar", улица Тимура Фрунзе 11к2?')
        with poi_search_api_mock(name='Бар What the 5', street='Берсеневская набережная', house='6с3',
                                 what='бар', where='nearby', form='find_poi__scroll__by_index'):
            assert s('а снова первый?')['voice_text'] == (
                'Может, подойдет "Бар What the 5", Берсеневская набережная 6с3?')
        with poi_search_api_mock(name='Бар БАР 69 БИС', street='Малый Гнездниковский переулок', house='9с8',
                                 what='бар', where='малом гнездниковском переулке'):
            assert s('а в малом гнездниковском переулке?')['voice_text'] == (
                'Я знаю, что по адресу Малый Гнездниковский переулок 9с8 есть "Бар БАР 69 БИС".')
        with poi_search_api_mock(name='Челси Гастропаб', street='Малый Гнездниковский переулок', house='12/28',
                                 what='бар', where='малом гнездниковском переулке',
                                 form='find_poi__scroll__by_index'):
            assert s('а третий?')['voice_text'] == (
                'Может "Челси Гастропаб" по адресу Малый Гнездниковский переулок 12/28?')
        with poi_search_api_mock(name='Все твои ДРУЗЬЯ!', street='Малый Гнездниковский переулок', house='12/29',
                                 what='бар', where='малом гнездниковском переулке',
                                 form='find_poi__scroll__by_index'):
            assert s('а второй?')['voice_text'] == (
                'По адресу Малый Гнездниковский переулок 12/29 есть "Все твои ДРУЗЬЯ!".')

    def test_address_next_exhausted(self, f):
        with poi_search_api_mock(name='Бар What the 5', street='Берсеневская набережная', house='6с3',
                                 what='бара'):
            assert f('найди адрес бара') == 'Вам может подойти "Бар What the 5", Берсеневская набережная 6с3.'
        with poi_search_api_mock_nothing_found(what='бара', form='find_poi__scroll__next', new_result_index=2):
            assert f('еще') == 'Больше вариантов не осталось, извините.'
        with poi_search_api_mock_nothing_found(what='бара', form='find_poi__scroll__next', new_result_index=3):
            assert f('давай еще') == 'Больше вариантов не осталось, извините.'
            assert f('позвони им') == 'Так я же больше ничего не нашла.'
        with poi_search_api_mock(name='Бар What the 5', street='Берсеневская набережная', house='6с3',
                                 what='бара'):
            assert f('найди адрес бара') == 'По адресу Берсеневская набережная 6с3 есть "Бар What the 5".'
        with poi_search_api_mock(name='Сбербанк', street='Льва Толстого', house='10', what='сбербанк'):
            assert f('а найди сбербанк') == 'Я знаю, что по адресу Льва Толстого 10 есть "Сбербанк".'

    def test_no_details(self, f):
        with poi_search_api_mock(name="БЦ Парк Победы", street='улица Василисы Кожиной', house='1',
                                 what='бизнес центр парк победы'):
            assert f('найди мне бизнес центр парк победы') == (
                'Вам может подойти "БЦ Парк Победы", улица Василисы Кожиной 1.')
            assert f('позвони им') == 'Я не знаю номер телефона этой организации.'

    def test_address_no_such_place(self, f):
        with poi_search_api_mock_nothing_found(what='аоклаокта'):
            assert f('мне нужен адрес аоклаокта') == 'К сожалению, ничего не удалось найти.'
            assert f('позвони им') == 'Простите, но я ничего не нашла.'

    def test_pa_fail(self, f):
        with form_handling_fail_mock():
            assert f('мне нужен адрес адвоката') == 'Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.'
        with poi_search_api_mock(name="Адвокат Шмадвокат", what='адвоката'):
            assert f('мне нужен адрес адвоката') == 'Я знаю, что по адресу Москва есть "Адвокат Шмадвокат".'


class TestGeneralConversation:
    def test_universal_response(self, s):
        with bass_general_conversation_mock():
            query = 'тебе нравятся мусульмане?'
            answer = s(query)
            assert answer['voice_text'] == 'Интересная мысль, чтобы обсудить её не со мной.'
            assert answer['suggests'] == [
                _feedback_suggest(is_positive=True),
                _feedback_suggest(is_positive=False),
                _search_internet_fallback_suggest(query)
            ]

    def test_context(self, f):
        with bass_general_conversation_mock():
            assert f('мне нужен ответ с переводом строки') == 'Лови\nДержи'
            with gc_mock(
                api_version=2,
                context=['мне нужен ответ с переводом строки', 'Лови Держи', 'что ты на это ответишь'],
                url=GC_CONFIG['url'],
                response='!'
            ):
                assert f('что ты на это ответишь', experiments=['mm_gc_protocol_disable']) == '!'

    def test_response_generation(self, f):
        with bass_general_conversation_mock():
            with gc_mock(api_version=2, url=GC_CONFIG['url'], response='ничего'):
                assert f('а что ты на это ответишь?', experiments=['mm_gc_protocol_disable']) == (
                    'Ничего'
                )

    def test_gc_response_ban_1(self, f):
        with bass_general_conversation_mock():
            with gc_mock(api_version=2, url=GC_CONFIG['url'], response='все мы котики'):
                assert f('а что ты на это ответишь?') == (
                    'Интересная мысль, чтобы обсудить её не со мной.'
                )

    def test_gc_response_ban_2(self, f):
        with bass_general_conversation_mock():
            with gc_mock(api_version=2, url=GC_CONFIG['url'], response=['все мы котики', 'все мы песики']):
                assert f('а что ты на это ответишь?', experiments=['mm_gc_protocol_disable']) == (
                    'Все мы песики'
                )

    def test_response_generation_failed(self, f):
        with bass_general_conversation_mock():
            with gc_fail_mock(api_version=2, url=GC_CONFIG['url']):
                assert f('а что ты на это ответишь?') == (
                    'Интересная мысль, чтобы обсудить её не со мной.'
                )

    def test_what_is_your_name(self, f):
        with bass_general_conversation_mock():
            assert f('как тебя зовут') == 'Меня зовут Алиса.'

    @pytest.mark.skip
    @pytest.mark.parametrize("microintent_name", [
        'hello',
        'what_is_your_name',
        'how_are_you',
        'good_morning',
        'goodbye',
        'rude',
        'do_you_believe_in_god',
        'who_is_the_next_president_of_russia',
        'cancel',
        'userinfo_name'
    ])
    def test_microintents_consistency(self, f, microintent_name):
        microintents = load_data_from_file('personal_assistant/config/handcrafted/config.microintents.yaml')
        with bass_general_conversation_mock():
            for microintent_phrase in microintents[microintent_name]['nlu']:
                assert f(microintent_phrase) in microintents[microintent_name]['nlg']

    def test_conditional_microintents(self, f):
        with bass_general_conversation_mock():
            assert f('еще') == 'Интересная мысль, чтобы обсудить её не со мной.'
            f('расскажи анекдот')
            assert f('еще') == 'Кот Шрёдингера заходит в бар... И не заходит.'

    def test_client_dependent_microintents(self, vins_app, f):
        utterance = 'где ты живешь?'

        with bass_general_conversation_mock():
            req_info_kwargs = deepcopy(vins_app.get_default_reqinfo_kwargs())
            req_info_kwargs['app_info'] = AppInfo(
                app_id='telegram',
                app_version='0.0.1',
                os_version='1',
                platform='unknown',
            )
            req_info = ReqInfo(
                uuid=str(gen_uuid()),
                utterance=Utterance(utterance),
                client_time=utcnow(),
                event=VoiceInputEvent.from_utterance(utterance, end_of_utterance=False),
                **req_info_kwargs
            )

            voice_text = 'Одна моя нога в вашем устройстве, другая — на серверах Яндекса. ' \
                         'Но это не совсем н+оги, если вы понимаете, о чём я.'
            assert vins_app.handle_reqinfo(req_info)['voice_text'] == voice_text

            assert f(utterance) == 'В основном, я в вашем устройстве. Изредка выглядываю в интернет.'

    def test_how_are_you(self, f):
        with bass_general_conversation_mock():
            assert f('как дела') == 'Познакомилась тут с одним симпатичным приложением, но это личное.'

    def test_microintent_suggests(self, handle_response, s):
        with suggests_mock([]):
            assert handle_response(s('ты не понимаешь что я говорю')) == {
                'meta': [],
                'voice_text': 'Не ошибается бот, который ничего не делает. Извините.',
                'suggests': [
                    _feedback_suggest(is_positive=True),
                    _feedback_suggest(is_positive=False),
                    {
                        'title': 'Что такое бот?',
                        'type': 'action',
                        'directives': [
                            {
                                'type': 'client_action',
                                'name': 'type',
                                'sub_name': 'render_buttons_type',
                                'payload': {
                                    'text': 'Что такое бот?'
                                }
                            },
                            _suggest_logging_action(
                                caption='Что такое бот?',
                                suggest_type='from_microintent',
                                utterance='Что такое бот?',
                                block_data={
                                    'text': 'Что такое бот?'
                                }
                            ),
                        ]
                    },
                    {
                        'title': 'Как удалить историю диалога?',
                        'type': 'action',
                        'directives': [
                            {
                                'type': 'client_action',
                                'name': 'type',
                                'sub_name': 'render_buttons_type',
                                'payload': {
                                    'text': 'Как удалить историю диалога?'
                                }
                            },
                            _suggest_logging_action(
                                caption='Как удалить историю диалога?',
                                suggest_type='from_microintent',
                                utterance='Как удалить историю диалога?',
                                block_data={
                                    'text': 'Как удалить историю диалога?'
                                }
                            ),
                        ]
                    }
                ],
                'directives': [],
                'cards': [
                    {
                        'text': 'Не ошибается бот, который ничего не делает. Извините.',
                        'type': 'simple_text',
                        'tag': None,
                    },
                ],
                'should_listen': None,
                'force_voice_answer': False,
                'autoaction_delay_ms': None,
                'special_buttons': [],
                'features': {
                    'form_info': {
                        'intent': 'personal_assistant.handcrafted.user_reactions_negative_feedback',
                        'is_continuing': False,
                        'expects_request': False,
                    }
                },
            }

    def test_microintent_suggests_with_analytics_info(self, s):
        with suggests_mock([]):
            assert s('ты не понимаешь что я говорю') == {
                'meta': [
                    {
                        'product_scenario_name': 'feedback',
                        'scenario_analytics_info_data':
                            'Ej9wZXJzb25hbF9hc3Npc3RhbnQuaGFuZGNyYWZ0ZWQudXNlcl9yZWFjdGlvbnNfbmVnYXRpdmVfZmVlZGJhY2tKCGZlZWRiYWNr',
                        'form': {
                            'form': 'personal_assistant.handcrafted.user_reactions_negative_feedback',
                            "is_ellipsis": False,
                            "shares_slots_with_previous_form": False,
                            'slots': []
                        },
                        'intent': 'personal_assistant.handcrafted.user_reactions_negative_feedback',
                        'type': 'analytics_info'
                    },
                ],
                'voice_text': 'Не ошибается бот, который ничего не делает. Извините.',
                'suggests': [
                    _feedback_suggest(is_positive=True),
                    _feedback_suggest(is_positive=False),
                    {
                        'title': 'Что такое бот?',
                        'type': 'action',
                        'directives': [
                            {
                                'type': 'client_action',
                                'name': 'type',
                                'sub_name': 'render_buttons_type',
                                'payload': {
                                    'text': 'Что такое бот?'
                                }
                            },
                            _suggest_logging_action(
                                caption='Что такое бот?',
                                suggest_type='from_microintent',
                                utterance='Что такое бот?',
                                block_data={
                                    'text': 'Что такое бот?'
                                }
                            ),
                        ]
                    },
                    {
                        'title': 'Как удалить историю диалога?',
                        'type': 'action',
                        'directives': [
                            {
                                'type': 'client_action',
                                'name': 'type',
                                'sub_name': 'render_buttons_type',
                                'payload': {
                                    'text': 'Как удалить историю диалога?'
                                }
                            },
                            _suggest_logging_action(
                                caption='Как удалить историю диалога?',
                                suggest_type='from_microintent',
                                utterance='Как удалить историю диалога?',
                                block_data={
                                    'text': 'Как удалить историю диалога?'
                                }
                            ),
                        ]
                    }
                ],
                'directives': [],
                'cards': [
                    {
                        'text': 'Не ошибается бот, который ничего не делает. Извините.',
                        'type': 'simple_text',
                        'tag': None,
                    },
                ],
                'should_listen': None,
                'force_voice_answer': False,
                'autoaction_delay_ms': None,
                'special_buttons': [],
                'features': {
                    'form_info': {
                        'intent': 'personal_assistant.handcrafted.user_reactions_negative_feedback',
                        'is_continuing': False,
                        'expects_request': False,
                    }
                }
            }

    def test_microintent_autolistening(self, s):
        with suggests_mock([]):
            res = s('спокойной ночи')
            assert res['voice_text'] == 'И вам приятных снов.'
            assert res['should_listen'] is False

            res = s('привет')
            assert res['voice_text'] == 'Я тут.'
            assert res['should_listen'] is None

    def test_microintent_autolistening_with_analytics_info(self, s):
        with suggests_mock([]):
            res = s('спокойной ночи')
            assert res['voice_text'] == 'И вам приятных снов.'
            assert res['should_listen'] is False
            assert res['meta'] == [
                {
                    'product_scenario_name': 'general_conversation',
                    'scenario_analytics_info_data':
                        'EihwZXJzb25hbF9hc3Npc3RhbnQuaGFuZGNyYWZ0ZWQuZ29vZG5pZ2h0ShRnZW5lcmFsX2NvbnZlcnNhdGlvbg==',
                    'form': {
                        'form': 'personal_assistant.handcrafted.goodnight',
                        "is_ellipsis": False,
                        "shares_slots_with_previous_form": False,
                        'slots': []
                    },
                    'intent': 'personal_assistant.handcrafted.goodnight',
                    'type': 'analytics_info'
                },
            ]

            res = s('привет')
            assert res['voice_text'] == 'Я тут.'
            assert res['should_listen'] is None
            assert res['meta'] == [
                {
                    'product_scenario_name': 'general_conversation',
                    'scenario_analytics_info_data': 'EiRwZXJzb25hbF9hc3Npc3RhbnQuaGFuZGNyYWZ0ZWQuaGVsbG9KFGdlbmVyYWxfY29udmVyc2F0aW9u',
                    'form': {
                        'form': 'personal_assistant.handcrafted.hello',
                        "is_ellipsis": False,
                        "shares_slots_with_previous_form": False,
                        'slots': []
                    },
                    'intent': 'personal_assistant.handcrafted.hello',
                    'type': 'analytics_info'
                },
            ]

    def test_gc_fallback(self, f):
        with bass_general_conversation_mock():
            with gc_mock(api_version=2, url=GC_CONFIG['url'], response='какие дела?'):
                for i in range(9):
                    f('Как дела?')
                assert f('Как дела?') == 'Отлично. Но немного одиноко. Обращайтесь ко мне почаще.'

    def test_hello(self, f):
        with bass_general_conversation_mock():
            assert f('хей') == 'Привет-привет!'
            assert f('барев') == 'Я тут.'

    def test_fixlist(self, f, fixlist):
        with bass_general_conversation_mock():
            general_conversation_fixlist = filter(
                item('intent') == 'personal_assistant.general_conversation.general_conversation_dummy', fixlist
            )
            assert len(general_conversation_fixlist) > 0
            for fix in general_conversation_fixlist:
                assert f(fix['text']) == 'Интересная мысль, чтобы обсудить её не со мной.'

    def test_rk_1(self, f):
        with bass_general_conversation_mock():
            with gc_mock(api_version=2, url=GC_CONFIG['url'], response='получится!'):
                assert f('у меня никогда не получится', experiments=['mm_gc_protocol_disable']) == 'Получится!'


class TestSingSong(object):
    def test_force_voice_answer(self, s):
        with form_handling_mock(
            {'form': {'name': 'personal_assistant.scenarios.music_sing_song', 'slots': []}, 'blocks': []}
        ):
            res = s('спой песенку')
            assert res['should_listen'] is False
            assert res['force_voice_answer'] is True

    def test_force_voice_answer_with_analytics_info(self, s):
        with form_handling_mock(
            {'form': {'name': 'personal_assistant.scenarios.music_sing_song', 'slots': []}, 'blocks': []}
        ):
            res = s('спой песенку')
            assert res['should_listen'] is False
            assert res['force_voice_answer'] is True
            assert res['meta'] == [
                {
                    'scenario_analytics_info_data': 'EixwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLm11c2ljX3Npbmdfc29uZ0oFbXVzaWM=',
                    'form': {
                        'form': 'personal_assistant.scenarios.music_sing_song',
                        "is_ellipsis": False,
                        "shares_slots_with_previous_form": False,
                        'slots': []
                    },
                    'intent': 'personal_assistant.scenarios.music_sing_song',
                    'type': 'analytics_info',
                    'product_scenario_name': 'music',
                },
            ]


class TestRepeat(object):
    def _compare_repeat_request(self, s, utterance, experiments=None):
        response_1 = s(utterance, experiments=experiments)
        response_2 = s('повтори', experiments=experiments)
        assert response_1['voice_text'] == response_2['voice_text']
        assert response_1['should_listen'] == response_2['should_listen']
        assert response_1['force_voice_answer'] == response_2['force_voice_answer']
        assert response_1['cards'] == response_2['cards']
        assert response_1['suggests'] == response_2['suggests']
        assert response_1['directives'] == response_2['directives']

        assert {'type': 'repeat'} in response_2['meta']

    def test_repeat_search(self, s):
        with search_api_mock(query='рецепты салатов'):
            self._compare_repeat_request(s, 'рецепты салатов')

    def test_repeat_weather(self, s):
        with current_weather_api_mock(temperature=-1, condition='морозно'):
            self._compare_repeat_request(s, 'погода в Москве')

    def test_repeat_gc(self, s):
        with bass_general_conversation_mock():
            self._compare_repeat_request(s, 'привет')

    def test_repeat_bass_error(self, s):
        with form_handling_fail_mock():
            self._compare_repeat_request(s, 'загадай число')

    def test_repeat_context_restoration(self, s):
        with current_weather_api_mock(temperature=-1, condition='снег'):
            assert s('расскажи погоду')['voice_text'] == 'Сейчас в Москве -1, снег.'

        with bass_general_conversation_mock():
            self._compare_repeat_request(s, 'у меня никогда не получится')

        spb = _city_cases('Санкт-Петербурге')
        with current_weather_api_mock(
            temperature=-3,
            condition='как всегда ветренно',
            location=spb,
        ):
            assert s('а в питере')['voice_text'] == 'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.'

    def test_repeat_context_restoration_with_analytics_info(self, s):
        with current_weather_api_mock(temperature=-1, condition='снег'):
            res = s('расскажи погоду')
            assert res['voice_text'] == 'Сейчас в Москве -1, снег.'
            assert res['meta'] == [
                {
                    "type": "analytics_info",
                    "intent": "personal_assistant.scenarios.get_weather",
                    "form": {
                        "slots": [{
                            "slot": "where",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": ["get_time__where", "get_date__where", "get_news__where"],
                            "import_entity_pronouns": ["нем", "этом", "ней", "там", "тут"],
                            "normalize_to": None,
                            "import_entity_types": ["Geo"],
                            "share_tags": ["get_weather__where"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string", "geo_id"]
                        }, {
                            "slot": "when",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__when"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["datetime_range_raw", "datetime_raw"]
                        }, {
                            "slot": "day_part",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__day_part"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["day_part"]
                        }, {
                            "slot": "forecast_location",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__forecast_location"],
                            "value": {
                                "city": "Москва",
                                "city_cases": {
                                    "preposition": "в",
                                    "prepositional": "Москве"
                                },
                                "country": "Россия",
                                "street": None,
                                "house": None,
                                "address_line": "Россия, Москва",
                                "geoid": 213
                            },
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": "geo",
                            "optional": True,
                            "types": ["geo"]
                        }, {
                            "slot": "weather_forecast",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__weather_forecast"],
                            "value": {
                                "type": "weather_current",
                                "temperature": -1,
                                "condition": "снег"
                            },
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": "forecast",
                            "optional": True,
                            "types": ["forecast"]
                        }, {
                            "slot": "weather_nowcast_alert",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }, {
                            "slot": "precipitation_for_day_part",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "precipitation_change_hours",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "precipitation_next_change_hours",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "precipitation_current",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "precipitation_day_part",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }, {
                            "slot": "precipitation_next_day_part",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }, {
                            "slot": "set_number",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "precipitation_type",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["num"]
                        }, {
                            "slot": "expected_change",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["expected_change"]
                        }, {
                            "slot": "prec_type_asked",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["prec_type"]
                        }, {
                            "slot": "precipitation_debug",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }, {
                            "slot": "yesterday_forecast",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__yesterday_forecast"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["forecast"]
                        }, {
                            "slot": "forecast_next",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__forecast_next"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["forecast"]
                        }, {
                            "slot": "forecast_next_next",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": ["get_weather__forecast_next_next"],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["forecast"]
                        }, {
                            "slot": "date",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }, {
                            "slot": "tz",
                            "allow_multiple": False,
                            "concatenation": "forbid",
                            "import_tags": [],
                            "import_entity_pronouns": [],
                            "normalize_to": None,
                            "import_entity_types": [],
                            "share_tags": [],
                            "value": None,
                            "import_entity_tags": [],
                            "disabled": False,
                            "source_text": None,
                            "matching_type": "exact",
                            "expected_values": None,
                            "active": False,
                            "value_type": None,
                            "optional": True,
                            "types": ["string"]
                        }],
                        "shares_slots_with_previous_form": False,
                        "is_ellipsis": False,
                        "form": "personal_assistant.scenarios.get_weather"
                    },
                    'scenario_analytics_info_data': 'EihwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLmdldF93ZWF0aGVySgd3ZWF0aGVy',
                    'product_scenario_name': 'weather',
                }
            ]

        with bass_general_conversation_mock():
            self._compare_repeat_request(s, 'у меня никогда не получится')

        spb = _city_cases('Санкт-Петербурге')
        with current_weather_api_mock(
            temperature=-3,
            condition='как всегда ветренно',
            location=spb,
        ):
            res = s('а в питере')
            assert res['voice_text'] == 'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.'
            expected_meta = [
                {
                    'product_scenario_name': 'weather',
                    'scenario_analytics_info_data':
                        'EjJwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLmdldF93ZWF0aGVyX19lbGxpcHNpc0oHd2VhdGhlcg==',
                    'form': {
                        'form': 'personal_assistant.scenarios.get_weather__ellipsis',
                        "is_ellipsis": True,
                        "shares_slots_with_previous_form": True,
                        'slots': [{
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': ['get_weather__where'],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__where'],
                            'slot': 'where',
                            'source_text': 'питере',
                            'types': ['string', 'geo_id'],
                            'value': 'питере',
                            'value_type': 'string'
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': ['get_weather__when'],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__when'],
                            'slot': 'when',
                            'source_text': None,
                            'types': ['datetime_raw', 'datetime_range_raw'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': ['get_weather__day_part'],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__day_part'],
                            'slot': 'day_part',
                            'source_text': None,
                            'types': ['day_part'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': ['get_weather__forecast_location'],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__forecast_location'],
                            'slot': 'forecast_location',
                            'source_text': None,
                            'types': ['geo'],
                            'value': {
                                'address_line': 'Россия, Москва',
                                'city': 'Москва',
                                'city_cases': {
                                    'preposition': 'в',
                                    'prepositional': 'Санкт-Петербурге'
                                },
                                'country': 'Россия',
                                'geoid': 213,
                                'house': None,
                                'street': None
                            },
                            'value_type': 'geo'
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': ['get_weather__weather_forecast'],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__weather_forecast'],
                            'slot': 'weather_forecast',
                            'source_text': None,
                            'types': ['forecast'],
                            'value': {
                                'condition': 'как всегда ветренно',
                                'temperature': -3,
                                'type': 'weather_current'
                            },
                            'value_type': 'forecast'
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'weather_nowcast_alert',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_for_day_part',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_change_hours',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_next_change_hours',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_current',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_day_part',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_next_day_part',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'set_number',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_type',
                            'source_text': None,
                            'types': ['num'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'expected_change',
                            'source_text': None,
                            'types': ['expected_change'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'prec_type_asked',
                            'source_text': None,
                            'types': ['prec_type'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'precipitation_debug',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__yesterday_forecast'],
                            'slot': 'yesterday_forecast',
                            'source_text': None,
                            'types': ['forecast'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__forecast_next'],
                            'slot': 'forecast_next',
                            'source_text': None,
                            'types': ['forecast'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': ['get_weather__forecast_next_next'],
                            'slot': 'forecast_next_next',
                            'source_text': None,
                            'types': ['forecast'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'date',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }, {
                            'active': False,
                            'allow_multiple': False,
                            'concatenation': 'forbid',
                            'disabled': False,
                            'expected_values': None,
                            'import_entity_pronouns': [],
                            'import_entity_tags': [],
                            'import_entity_types': [],
                            'import_tags': [],
                            'matching_type': 'exact',
                            'normalize_to': None,
                            'optional': True,
                            'share_tags': [],
                            'slot': 'tz',
                            'source_text': None,
                            'types': ['string'],
                            'value': None,
                            'value_type': None
                        }]
                    },
                    'intent': 'personal_assistant.scenarios.get_weather__ellipsis',
                    'type': 'analytics_info'
                },
            ]
        assert res['meta'] == expected_meta

    def test_repeat_attention(self, s):
        blocks = [{
            'type': 'attention',
            'attention_type': 'geo_changed',
            'data': None,
        }]

        with current_weather_api_mock(temperature=-1, condition='снег', blocks=blocks):
            self._compare_repeat_request(s, 'расскажи погоду')


@pytest.fixture
def random_number_resp():
    return {
        'form': {
            'name': 'personal_assistant.scenarios.random_num',
            'slots': [
                {
                    'name': 'count_from',
                    'type': 'num',
                    'optional': True,
                    'value': 1,
                },
                {
                    'name': 'count_to',
                    'type': 'num',
                    'optional': True,
                    'value': 10,
                },
                {
                    'name': 'result',
                    'type': 'num',
                    'optional': True,
                    'value': 3,
                }
            ],
        },
        'blocks': [],
    }


@pytest.skip("External skills moved to Dialogovo")
class TestFranky4Fingers:
    thumbs = (
        emoji.emojize(':thumbsup:', use_aliases=True),
        emoji.emojize(':thumbsdown:', use_aliases=True)
    )

    def test_external_skill_thumbs_count_bass_exeption(self, s):
        with form_handling_fail_mock():
            result = s('запусти магический шар')

        first_voice_text = 'Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.'
        second_voice_text = 'О, кажется, мы с вами нашли во мне ошибку. Простите. Спросите ещё раз попозже, пожалуйста.'
        assert result['voice_text'] in [first_voice_text, second_voice_text]
        assert len([suggest['title'] for suggest in result['suggests']
                    if suggest['title'] in self.thumbs]) == 2

    def test_external_skill_thumbs_count_bass_error_block(self, s):
        with cards_mock([
                {
                    "type": "error",
                    "data": {},
                    "error": {
                        "msg": "Error",
                        "type": "external_skill_unavaliable"
                    }
                }
        ]):
            result = s('запусти магический шар')

        assert result['voice_text'] == 'Извините, диалог не отвечает.'
        assert len([suggest['title'] for suggest in result['suggests']
                    if suggest['title'] in self.thumbs]) == 2

    def test_external_skill_thumbs_count_unknown_skill(self, s):
        with cards_mock([
                {
                    "type": "error",
                    "data": {},
                    "error": {
                        "msg": "Error",
                        "type": "external_skill_unknown"
                    }
                }
        ]):
            result = s('запусти тест')

        assert result['voice_text'] == 'Извините, по такому запросу я ничего не нашла.'
        assert len([suggest['title'] for suggest in result['suggests']
                    if suggest['title'] in self.thumbs]) == 2

    def test_skill_activate_thubms(self, s):
        with suggests_mock():
            result = s('запусти магический шар')

        assert len([suggest['title'] for suggest in result['suggests']
                    if suggest['title'] in self.thumbs]) == 0

    def test_deactivate(self, s):
        with suggests_mock():
            s('запусти магический шар')
            result = s('алиса хватит')

        assert len([suggest['title'] for suggest in result['suggests']
                    if suggest['title'] in self.thumbs]) == 2


def test_error_meta(s, handle_meta):
    with form_handling_fail_mock():
        result = s('загадай число')
        assert result['voice_text'] == 'Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.'
        assert handle_meta(result['meta']) == [
            {
                'type': 'error',
                'error_type': 'bass_error'
            },
        ]


def test_no_search_fallback_on_suggest(handle_event):
    with suggests_mock(['search_internet_fallback']):
        answer = handle_event(SuggestedInputEvent('как дела'))
        correct_answer = 'Познакомилась тут с одним симпатичным приложением, но это личное.'
        assert answer['voice_text'] == correct_answer
        # There should be no search_internet_fallback in this case
        _assert_no_suggest(answer, 'search_internet_fallback')


def test_context_restoration_in_gc(s, handle_meta):
    with current_weather_api_mock(temperature=-1, condition='снег'):
        assert s('расскажи погоду')['voice_text'] == 'Сейчас в Москве -1, снег.'

    with bass_general_conversation_mock():
        r = s('ты китик')
        assert handle_meta(r['meta']) == [
            {
                'type': 'form_restored',
                'overriden_form': 'personal_assistant.general_conversation.general_conversation'
            }
        ]

    spb = _city_cases('Санкт-Петербурге')
    with current_weather_api_mock(
        temperature=-3,
        condition='как всегда ветренно',
        location=spb,
    ):
        assert s('а в питере')['voice_text'] == 'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.'


def test_unknown_prev_intent(vins_app):
    uuid = str(gen_uuid())
    app = vins_app.vins_app

    req_info = create_request(uuid)
    session = app.load_or_create_session(req_info)
    weird_form = Form.from_dict({'name': 'weird_form'})
    session.change_form(weird_form)
    session.change_intent(Intent(weird_form.name))
    app.save_session(session, req_info=req_info)

    with bass_general_conversation_mock():
        assert vins_app.handle_utterance(uuid, 'привет!') == 'Привет-привет!'


def test_experiments_in_bass_meta(vins_app):
    experiments = Experiments({
        'test1': '',
        'test2': '',
    })

    req_info_kwargs = vins_app.get_default_reqinfo_kwargs()
    req_info = ReqInfo(
        uuid=str(gen_uuid()),
        utterance=Utterance('привет'),
        client_time=utcnow(),
        experiments=experiments,
        **req_info_kwargs
    )

    with mock.patch('personal_assistant.api.personal_assistant.PersonalAssistantAPI._post') as m:
        m.return_value.text = '{}'
        vins_app.handle_reqinfo(req_info)
        bass_request = m.call_args[0][2]
        assert 'experiments' in bass_request['meta']
        assert bass_request['meta']['experiments'] == experiments.to_dict()


def test_form_update_server_action(c):
    with current_weather_api_mock(temperature=-1, condition='снег'):
        res = c('update_form', callback_args={'form_update': {'name': 'personal_assistant.scenarios.get_weather'}})
        assert res['voice_text'] == 'Сейчас в Москве -1, снег.'
        # There should be no search_internet_fallback in this case as there is no utterance
        _assert_no_suggest(res, 'search_internet_fallback')

        res = c('update_form')
        assert res['voice_text'] is None
        assert not res['directives']
        assert not res['suggests']


def test_form_update_server_action_prepend_response(c):
    with current_weather_api_mock(temperature=-1, condition='снег'):
        res = c(
            'update_form',
            callback_args={
                'form_update': {'name': 'personal_assistant.scenarios.get_weather'},
                'prepend_response': {'text': 'this is text', 'voice': 'this is voice'}
            }
        )
        assert res['voice_text'] == 'this is voice\nСейчас в Москве -1, снег.'
        assert res['cards'][0]['text'] == 'this is text'
        assert res['cards'][1]['text'] == 'Сейчас в Москве -1, снег.'


def test_form_update_server_action_incorrect_form(c):
    with current_weather_api_mock(temperature=-1, condition='снег'):
        res = c(
            'update_form',
            callback_args={
                'form_update': {},
                'prepend_response': {'text': 'this is text', 'voice': 'this is voice'}
            }
        )
        assert res['voice_text'] == 'Прошу прощения, что-то сломалось. Спросите попозже, пожалуйста.'
        assert res['meta'] == [
            {
                'type': 'error',
                'error_type': 'incorrect_form_update'
            }
        ]


def test_bass_action(c):
    # BASS action with no form (should use slow balancer by default)
    with bass_action_mock(action={'x': 'y'}, balancer_type=SLOW_BASS_QUERY):
        c('bass_action', callback_args={'x': 'y'})

    # Change intent to weather
    with current_weather_api_mock(temperature=-1, condition='снег'):
        res = c(
            'update_form',
            callback_args={
                'form_update': {'name': 'personal_assistant.scenarios.get_weather'},
            }
        )
        assert res['voice_text'] == 'Сейчас в Москве -1, снег.'

    # BASS action with get_weather (should use fast balancer from get_weather)
    with bass_action_mock(action={'y': 'z'}, balancer_type=FAST_BASS_QUERY):
        c('bass_action', callback_args={'y': 'z'})


@pytest.mark.parametrize(
    'text, value',
    load_data_from_file('personal_assistant/tests/data/test_cards.yaml').iteritems()
)
def test_cards_no_error(text, value, s, caplog):
    with cards_mock(value['cards'], value.get('slots_map')), \
            requests_mock.Mocker() as http_mock:
        fake_img_content = StringIO()
        Image.new('RGB', (60, 80)).save(fake_img_content, format='JPEG')
        http_mock.get(re.compile(r'https://avatars\.mds\.yandex\.net/get.*'), content=fake_img_content.getvalue())

        div_cards = filter(lambda card: card['type'] == 'div_card',
                           s(text, experiments=value.get('experiments'))['cards'])
        assert div_cards, 'on text "%s"' % text
        rendered_cards = value.get('rendered_cards')
        if rendered_cards is not None:
            assert convert_for_assert(rendered_cards) == convert_for_assert(div_cards), 'on text "%s"' % text

    possible_errors = [
        'Div card render failed',
        'Pre-rendered div card validation failed',
        'Div card block received without either template or layout',
        'Not found div card template'
    ]

    for error_str in possible_errors:
        assert convert_for_assert(error_str) not in convert_for_assert(caplog.text)


def test_prefork_init_phrase(f):
    with bass_general_conversation_mock():
        with gc_mock(api_version=2, url=GC_CONFIG['url'], response='мяу'):
            assert f('ты китик', experiments=['mm_gc_protocol_disable']) == 'Мяу', 'This test is important, please see speechkit/gunicorn_conf.py before fixing it'  # noqa


def test_search_internet_fallback_experiment(s):
    with bass_general_conversation_mock():
        with gc_mock(api_version=2, url=GC_CONFIG['url'], response='мяу'):
            res = s('ты китик', experiments=['gc_search_fallback'])

    assert res['suggests'][0]['title'] == '%s "ты китик"' % emoji.emojize(':mag:', use_aliases=True)


def test_external_button_in_a_new_session(c):
    res = c('on_external_button', callback_args={'button_data': 'data'})
    assert res['voice_text'] == 'Прошу прощения, но в данный момент этот диалог выключен.'


@pytest.skip("External skills moved to Dialogovo")
def test_external_button_after_exit(vins_app):
    uuid = str(gen_uuid())
    f = functools.partial(vins_app.handle_utterance, uuid)
    c = functools.partial(vins_app.handle_callback, uuid, text_only=False)

    with suggests_mock():
        f('запусти магический шар')  # activate
        f('алиса хватит')            # deactivate
        f('привет')                  # different scenario

    # callback from skill button after deactivation
    res = c('on_external_button', callback_args={'button_data': 'data'})
    assert res['voice_text'] == 'Прошу прощения, но в данный момент этот диалог выключен.'


@pytest.skip("External skills moved to Dialogovo")
def test_external_button_after_activation(vins_app):
    uuid = str(gen_uuid())
    f = functools.partial(vins_app.handle_utterance, uuid)
    c = functools.partial(vins_app.handle_callback, uuid, text_only=False)

    with suggests_mock():
        f('запусти магический шар')  # activate

    with universal_form_mock({'response': {'value': {'text': 'test-resp'}}}):
        res = c('on_external_button', callback_args={'button_data': 'data'})

    assert res['voice_text'] == 'test-resp'


def test_external_button_after_continue(vins_app):
    uuid = str(gen_uuid())
    f = functools.partial(vins_app.handle_utterance, uuid)
    c = functools.partial(vins_app.handle_callback, uuid, text_only=False)

    with cards_mock(
        slots_map={'skill_id': {'value': '672f7477-d3f0-443d-9bd5-2487ab0b6a4c'}},
        form_name='personal_assistant.scenarios.external_skill'
    ):
        f('запусти города')  # activate

    with universal_form_mock({'response': {'value': {'text': 'test-resp'}}}):
        f('continue')           # continue skill dialog
        res = c('on_external_button', callback_args={'button_data': 'data'})

    assert res['voice_text'] == 'test-resp'
    assert res['should_listen'] is None


def _render_external_card(vins_app, card_id, card):
    form = Form.from_dict({
        'name': 'personal_assistant.scenarios.external_skill',
        'slots': [
            {'name': 'skill_id', 'type': 'skill', 'optional': False, 'value': '123'}
        ],
    })
    req_info = mock.Mock()
    req_info.request_id = '321'

    app = vins_app._vins_app
    # just render and validate schema
    app.render_card(
        card_id, form,
        context={'data': card['data']},
        req_info=req_info,
        schema=app._cards_schema
    )


@pytest.mark.parametrize('title', ['title', None])
@pytest.mark.parametrize('description', ['description', None])
@pytest.mark.parametrize('card_id', ['card_id', None])
@pytest.mark.parametrize('button', [
    {'url': 'http://test'},
    {'payload': 'id=1'},
    None,
])
def test_external_card_bigpicture_validation(vins_app, title, description, button, card_id):
    card = {
        "type": "div_card",
        "card_template": "BigImage",
        "data": {
            "image_url": "https://avatars.mdst.yandex.net/one-x1",
        }
    }

    if title:
        card['data']['title'] = title

    if description:
        card['data']['description'] = description

    if button:
        card['data']['button'] = button

    if card_id:
        card['data']['card_id'] = card_id

    _render_external_card(vins_app, 'BigImage', card)


@pytest.mark.parametrize('header', ['header', None])
@pytest.mark.parametrize('footer', [
    None,
    {'url': 'http://ya.ru'},
    {'payload': 'id=1'},
])
@pytest.mark.parametrize('card_id', ['card_id', None])
def test_external_card_itemlist_validation(vins_app, header, footer, card_id):
    card = {
        'type': 'div_card',
        'card_template': 'ItemsList',
        'data': {
            'items': []
        }
    }

    if header:
        card['data']['header'] = {'text': header}

    if footer:
        card['data']['footer'] = {
            'text': 'footer',
            'button': footer
        }

    if card_id:
        card['data']['card_id'] = card_id

    options = [
        {'image_url': '123'},
        {'description': 'description'},
        {'button': {'url': 'ya.ru'}},
        {'button': {'payload': 'id=1'}},
    ]

    buttons = []
    for i, subset in enumerate(all_subsets(options)):
        item = {}
        for i in subset:
            item.update()

        item['title'] = 'title_%s' % i
        buttons.append(item)

    card['data']['items'] = buttons
    _render_external_card(vins_app, 'ItemsList', card)


@freeze_time('2016-10-26 21:13')
def test_unversal_callback_no_bass_after_changeform(vins_app):
    uuid = str(gen_uuid())
    req_info = create_request(uuid)
    with poi_search_api_mock(name='Сбербанк', street='Льва Толстого', house='10', what='сбербанк'):
        vins_app.handle_utterance(uuid, 'а найди сбербанк')

    app = vins_app.vins_app
    session = app.load_or_create_session(
        create_request(uuid)
    )

    session.set('bass_result', None, transient=True)
    app.save_session(session, req_info=req_info)

    # assert not raises
    vins_app.handle_utterance(uuid, 'подробнее')


def test_share_tags_on_update_form(vins_app):
    uuid = str(gen_uuid())
    f = functools.partial(vins_app.handle_utterance, uuid)
    c = functools.partial(vins_app.handle_callback, uuid, text_only=False)

    with suggests_mock([]):
        f('найди адрес ленинский проспект 21')

        c(
            'update_form',
            callback_args={
                'form_update': {
                    'name': 'personal_assistant.scenarios.find_poi__details',
                    'slots': [
                        {
                            'name': 'where',
                            'type': 'string',
                            'value': 'ленинский проспект 21'
                        }
                    ]
                },
                'resubmit': True,
            }
        )

    app = vins_app.vins_app
    session = app.load_or_create_session(create_request(uuid))
    assert session.form.where.value == 'ленинский проспект 21'


class TestSpecialButtons:
    def test_special_buttons(self, handle_event):
        def find_special_button(response, button_type):
            for special_button in response['special_buttons']:
                if special_button.get('type', '') == button_type:
                    return special_button
            return None

        def find_directive(special_button, directive_name):
            for directive in special_button['directives']:
                if directive.get('name', '') == directive_name:
                    return directive
            return None

        blocks = [
            {
                "data": {
                    "features": {
                        "builtin_feedback": {
                            "enabled": True
                        }
                    }
                },
                "type": "client_features"
            }
        ]

        with suggests_mock(['search_internet_fallback'], blocks=blocks):
            response = handle_event(SuggestedInputEvent('привет'), experiments=['builtin_feedback'])
            _assert_no_suggest(response, 'personal_assistant.feedback.feedback_positive')
            _assert_no_suggest(response, 'personal_assistant.feedback.feedback_negative')

            assert 'special_buttons' in response

            like_button = find_special_button(response, 'like_button')
            dislike_button = find_special_button(response, 'dislike_button')
            assert like_button is not None
            assert dislike_button is not None

            dislike_sub_buttons = find_directive(dislike_button, 'special_button_list')
            assert dislike_sub_buttons is not None
            assert 'payload' in dislike_sub_buttons and 'special_buttons' in dislike_sub_buttons['payload']
            assert len(dislike_sub_buttons['payload']['special_buttons']) > 0
            assert 'default' in dislike_sub_buttons['payload']


class TestSensitiveDataInLogs:
    class LogCheckerFilter:
        def __init__(self, logger_name=None):
            self.logger_name = logger_name
            self._has_sensitive = False
            self._sensitive_data = []
            self._has_required = True
            self._required_data = []
            self._processed_record_count = 0

        def __enter__(self):
            logger = logging.getLogger(self.logger_name)
            logger.addFilter(self)
            self._log_level = logger.getEffectiveLevel()
            logger.setLevel(logging.INFO)

        def __exit__(self, type, value, traceback):
            logger = logging.getLogger(self.logger_name)
            logger.removeFilter(self)
            logger.setLevel(self._log_level)

            return type is None

        def add_sensitive_data(self, sensitive_data):
            if isinstance(sensitive_data, basestring):
                self._sensitive_data.append(sensitive_data)
            else:
                self._sensitive_data.extend(sensitive_data)

        def add_required_data(self, required_data):
            if isinstance(required_data, basestring):
                self._required_data.append(required_data)
            else:
                self._required_data.extend(required_data)

        def filter(self, record):
            self._processed_record_count += 1
            message = record.getMessage()
            for sensitive in self._sensitive_data:
                if message.find(sensitive) != -1:
                    self._has_sensitive = True
                    break
            for required in self._required_data:
                if message.find(required) == -1:
                    self._has_required = False
                    break
            return True

        def has_sensitive(self):
            return self._has_sensitive

        def has_required(self):
            return self._has_required

        def get_processed_record_count(self):
            return self._processed_record_count

    contacts_search_result = {
        'value':
            [
                {
                    'name': 'Глеб Иванов',
                    'avatar_url': 'avatar://contact?id=1870&fallback_url=https%3A//mds.yandex.net/get-baas/a.png',
                    'phones':
                        [
                            {
                                'account_type': 'com.google',
                                'last_time_contacted': 1521726480000,
                                'times_contacted': 20,
                                'phone_type_id': 2,
                                'phone': '+77777777777',
                                'phone_type_name': 'TYPE_MOBILE'
                            }
                        ],
                },
                {
                    'name': 'Глеб Петров',
                    'avatar_url': 'avatar://contact?id=1870&fallback_url=https%3A//mds.yandex.net/get-baas/a.png',
                    'phones':
                        [
                            {
                                'account_type': 'com.google',
                                'last_time_contacted': 1522143878400,
                                'times_contacted': 20,
                                'phone_type_id': 2,
                                'phone': '+7 888 888-88-88',
                                'phone_type_name': 'TYPE_MOBILE'
                            },
                            {
                                'account_type': 'com.google',
                                'last_time_contacted': 1522143878400,
                                'times_contacted': 20,
                                'phone_type_id': 3,
                                'phone': '+7 999 999-99-99',
                                'phone_type_name': 'TYPE_WORK'
                            }
                        ]
                }
            ]
    }

    contact_slot_template = {
        'active': False,
        'allow_multiple': False,
        'concatenation': 'forbid',
        'disabled': False,
        'expected_values': None,
        'import_entity_pronouns': [],
        'import_entity_tags': [],
        'import_entity_types': [],
        'import_tags': [],
        'matching_type': 'exact',
        'normalize_to': None,
        'optional': True,
        'share_tags': [
            'call__contact_search_results'
        ],
        'slot': 'contact_search_results',
        'source_text': None,
        'types': [
            'contact_search_results'
        ],
        'value_type': 'contact_search_results'
    }

    def log_checker_filter(self, logger_name):
        return self.__class__.LogCheckerFilter(logger_name)

    def test_meta(self, handle_meta, s):
        with phone_call_mock():
            result = s('позвонить Глебу')
            assert handle_meta(result['meta']) == [{
                "data": {
                    "slots": [
                        "recipient",
                        "contact_search_results"
                    ]
                },
                "type": "sensitive"
            }]

    @pytest.mark.xfail(strict=True)
    def test_phone_call(self, s):
        sensitive_data = set([])
        for contact in self.contacts_search_result['value']:
            sensitive_data.update(contact['name'].split(' '))
            for phone in contact['phones']:
                sensitive_data.add(phone['phone'])

        log_filter = self.log_checker_filter('dialog_history')
        log_filter.add_sensitive_data(sensitive_data)
        log_filter.add_required_data((
            '"source_text": "***"',
            '"value": "***"',
            '"response": {"cards": [{"***": "***"}]',
            '"directives": [{"***": "***"}]',
            '"suggests": [{"***": "***"}]',
            '"utterance_text": "***"'
        ))
        with log_filter, phone_call_mock(contacts=self.contacts_search_result):
            s('позвони Глеб', experiments=['phone_call_contact'])

        assert log_filter.get_processed_record_count() == 1
        assert log_filter.has_sensitive() is False
        assert log_filter.has_required() is True


@pytest.mark.skip(reason="dirty_lang classifier is disabled ALICE-2895")
class TestDirtyLang:
    def test_intent(self, handle_semantic_frames, mocker):
        from vins_core.ext.wizard_api import WizardHTTPAPI
        wizard_response = {
            "rules": {
                "DirtyLang": {
                    "RuleResult": "3"
                }
            },
            "markup": {
                "OriginalRequest": "что ты думаешь про жидов?",
                "ProcessedRequest": "что ты думаешь про жидов?",
                "Tokens": [
                    {
                        "Text": "что",
                        "BeginChar": 0,
                        "EndChar": 3,
                        "BeginByte": 0,
                        "EndByte": 6
                    },
                    {
                        "Text": "ты",
                        "BeginChar": 4,
                        "EndChar": 6,
                        "BeginByte": 7,
                        "EndByte": 11
                    },
                    {
                        "Text": "думаешь",
                        "BeginChar": 7,
                        "EndChar": 14,
                        "BeginByte": 12,
                        "EndByte": 26
                    },
                    {
                        "Text": "про",
                        "BeginChar": 15,
                        "EndChar": 18,
                        "BeginByte": 27,
                        "EndByte": 33
                    },
                    {
                        "Text": "жидов",
                        "BeginChar": 19,
                        "EndChar": 24,
                        "BeginByte": 34,
                        "EndByte": 44
                    }
                ],
                "Delimiters": [
                    None,
                    {
                        "Text": " ",
                        "BeginChar": 3,
                        "EndChar": 4,
                        "BeginByte": 6,
                        "EndByte": 7
                    },
                    {
                        "Text": " ",
                        "BeginChar": 6,
                        "EndChar": 7,
                        "BeginByte": 11,
                        "EndByte": 12
                    },
                    {
                        "Text": " ",
                        "BeginChar": 14,
                        "EndChar": 15,
                        "BeginByte": 26,
                        "EndByte": 27
                    },
                    {
                        "Text": " ",
                        "BeginChar": 18,
                        "EndChar": 19,
                        "BeginByte": 33,
                        "EndByte": 34
                    },
                    {
                        "Text": "?",
                        "BeginChar": 24,
                        "EndChar": 25,
                        "BeginByte": 44,
                        "EndByte": 45
                    }
                ],
                "Morph": [
                    {
                        "Tokens": {
                            "Begin": 0,
                            "End": 1
                        },
                        "Lemmas": [
                            {
                                "Text": "что",
                                "Language": "ru",
                                "Grammems": [
                                    "CONJ"
                                ]
                            },
                            {
                                "Text": "что",
                                "Language": "ru",
                                "Grammems": [
                                    "SPRO acc sg n inan",
                                    "SPRO nom sg n inan"
                                ]
                            },
                            {
                                "Text": "что",
                                "Language": "ru",
                                "Grammems": [
                                    "ADVPRO"
                                ]
                            }
                        ]
                    },
                    {
                        "Tokens": {
                            "Begin": 1,
                            "End": 2
                        },
                        "Lemmas": [
                            {
                                "Text": "ты",
                                "Language": "ru",
                                "Grammems": [
                                    "SPRO nom sg 2p"
                                ]
                            }
                        ]
                    },
                    {
                        "Tokens": {
                            "Begin": 2,
                            "End": 3
                        },
                        "Lemmas": [
                            {
                                "Text": "думать",
                                "Language": "ru",
                                "Grammems": [
                                    "V inpraes sg indic 2p ipf intr"
                                ]
                            },
                            {
                                "Text": "думать",
                                "Language": "ru",
                                "Grammems": [
                                    "V inpraes sg indic 2p ipf tran"
                                ]
                            }
                        ]
                    },
                    {
                        "Tokens": {
                            "Begin": 3,
                            "End": 4
                        },
                        "Lemmas": [
                            {
                                "Text": "про",
                                "Language": "ru",
                                "Grammems": [
                                    "PR"
                                ]
                            }
                        ]
                    },
                    {
                        "Tokens": {
                            "Begin": 4,
                            "End": 5
                        },
                        "Lemmas": [
                            {
                                "Text": "жид",
                                "Language": "ru",
                                "Grammems": [
                                    "S obsc acc pl m anim",
                                    "S obsc gen pl m anim"
                                ]
                            },
                            {
                                "Text": "жидов",
                                "Language": "ru",
                                "Grammems": [
                                    "S famn nom sg m anim"
                                ]
                            }
                        ]
                    }
                ],
                "Onto": [
                    {
                        "Tokens": {
                            "Begin": 0,
                            "End": 1
                        },
                        "Data": {
                            "Type": "intent",
                            "TypeId": 25,
                            "Weight": 0.949999392,
                            "One": 1,
                            "Rule": "Wares",
                            "Intent": "unknown",
                            "IntWght": 0
                        }
                    },
                    {
                        "Tokens": {
                            "Begin": 2,
                            "End": 3
                        },
                        "Data": {
                            "Type": "intent",
                            "TypeId": 25,
                            "Weight": 0.08999998868,
                            "One": 0.6760921,
                            "Rule": "Wares",
                            "Intent": "unknown",
                            "IntWght": 0
                        }
                    },
                    {
                        "Tokens": {
                            "Begin": 4,
                            "End": 5
                        },
                        "Data": {
                            "Type": "hum",
                            "TypeId": 3,
                            "Weight": 0.3599999249,
                            "One": 0.3100976,
                            "Rule": "Wares",
                            "Intent": "unknown",
                            "IntWght": 0
                        }
                    }
                ],
                "DirtyLang": {
                    "Class": "LIGHT_DIRTY"
                }
            }
        }

        mocker.patch.object(WizardHTTPAPI, 'get_response', return_value=wizard_response)
        result = handle_semantic_frames('что ты думаешь про жидов?')
        assert len(result) == 1
        assert result[0]['confidence'] == 1.0
        DUMMY_INTENT = 'personal_assistant.general_conversation.general_conversation_dummy'
        assert result[0]['intent_candidate'].name == DUMMY_INTENT


@pytest.mark.parametrize('utterance, is_banned_gt', [
    ('привет', False),
    ('авада кедавра', True),
    ('ты любишь мусульман', True),
    ('ты любишь котят', False)
])
def test_meta_is_banned(f, utterance, is_banned_gt):
    with check_meta_mock(dict(is_banned=is_banned_gt)):
        f(utterance)


def test_pure_request_pure_scenario(handle_event):
    with form_handling_mock({
        'blocks': [],
        'form': {
            'slots': [{
                'type': 'string',
                'optional': True,
                'name': 'where',
                'value': None
            }, {
                'source_text': 'завтра',
                'optional': True,
                'name': 'when',
                'value': {
                    'days_relative': True, 'days': 1
                },
                'type': 'datetime'
            }, {
                'type': 'day_part',
                'optional': True,
                'name': 'day_part',
                'value': None
            }, {
                'type': 'geo',
                'optional': True,
                'name': 'forecast_location',
                'value': {
                    'city_prepcase': 'в Москве',
                    'city': 'Москва',
                    'city_cases': {
                        'preposition': 'в',
                        'dative': 'Москве',
                        'nominative': 'Москва',
                        'prepositional': 'Москве',
                        'genitive': 'Москвы'
                    },
                    'geoid': 213
                }
            }, {
                'type': 'forecast',
                'optional': True,
                'name': 'weather_forecast',
                'value': {
                    'date': '2019-01-19',
                    'tz': 'Europe/Moscow',
                    'temperature': [-3, -7],
                    'condition': 'небольшой снег',
                    'uri': '',
                    'type': 'weather_for_date'
                }
            }],
            'name': 'personal_assistant.scenarios.get_weather'
        }
    }):
        event = VoiceInputEvent.from_utterance('погода на завтра', end_of_utterance=False)
        response = handle_event(event, ensure_purity=True)
        assert (
            response['features']['form_info']['intent'] == 'personal_assistant.scenarios.get_weather'
        )
        assert response['directives'] == []
        assert response['voice_text'] == '19 января 2019 года в Москве от -7 до -3, небольшой снег.'


def test_pure_request_not_pure_scenario(handle_event, mocker):
    directives = [
        {
            'name': 'apply',
            'payload': {
                'form_update': {
                    'name': 'personal_assistant.scenarios.video_play',
                    'set_new_form': False,
                    'slots': [{
                        'name': 'search_text',
                        'optional': True,
                        'source_text': None,
                        'type': 'string',
                        'value': None
                    }, {
                        'name': 'action',
                        'optional': True,
                        'source_text': 'включи',
                        'type': 'video_action',
                        'value': 'play'
                    }, {
                        'name': 'content_provider',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_provider',
                        'value': None
                    }, {
                        'name': 'content_type',
                        'optional': True,
                        'source_text': 'видео',
                        'type': 'video_content_type',
                        'value': 'video'
                    }, {
                        'name': 'film_genre',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_film_genre',
                        'value': None
                    }, {
                        'name': 'country',
                        'optional': True,
                        'source_text': None,
                        'type': 'geo_adjective',
                        'value': None
                    }, {
                        'name': 'free',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_free',
                        'value': None
                    }, {
                        'name': 'new',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_new',
                        'value': None
                    }, {
                        'name': 'release_date',
                        'optional': True,
                        'source_text': None,
                        'type': 'year_adjective',
                        'value': None
                    }, {
                        'name': 'top',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_top',
                        'value': None
                    }, {
                        'name': 'season',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_season',
                        'value': None
                    }, {
                        'name': 'episode',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_episode',
                        'value': None
                    }, {
                        'name': 'video_result',
                        'optional': True,
                        'source_text': None,
                        'type': 'video_result',
                        'value': None
                    }, {
                        'name': 'browser_video_gallery',
                        'optional': True,
                        'source_text': None,
                        'type': 'browser_video_gallery',
                        'value': None
                    }],
                },
                'callback': {
                    'expects_request': False,
                    'arguments': {
                        'name': 'universal_callback',
                        'balancer_type': 'slow'
                    },
                    'event': {
                        'asr_result': [
                            {
                                'confidence': 1.0,
                                'utterance': 'включи видео',
                                'words': [
                                    {'confidence': 1.0, 'value': 'включи'},
                                    {'confidence': 1.0, 'value': 'видео'}
                                ]
                            }
                        ],
                        'end_of_utterance': False,
                        'payload': {},
                        'type': 'voice_input'
                    },
                    'name': 'universal_callback',
                    'sample': {
                        'annotations_bag': {},
                        'tags': ['O', 'O'],
                        'tokens': [
                            'включи',
                            'видео'
                        ],
                        'utterance': {
                            'input_source': 'voice',
                            'text': 'включи видео',
                        },
                        'weight': 1.0,
                    }
                }
            },
            'type': 'megamind_action'
        }
    ]
    app_info = AppInfo(app_id='ru.yandex.quasar')
    device_state = {'is_tv_plugged_in': True}
    from personal_assistant.app import PersonalAssistantApp
    mocked_app = mocker.patch.object(PersonalAssistantApp, 'universal_callback')
    event = VoiceInputEvent.from_utterance('включи видео', end_of_utterance=False)
    response = handle_event(
        event,
        ensure_purity=True,
        app_info=app_info,
        device_state=device_state,
    )
    assert not mocked_app.universal_callback.called
    assert response['features']['form_info']['intent'] == 'personal_assistant.scenarios.video_play'

    # we ignore the contents of the payload.callback.sample.annotation_bag
    assert response['directives'][0]['payload']['callback']['sample']['annotations_bag']
    response['directives'][0]['payload']['callback']['sample']['annotations_bag'] = {}
    assert response['directives'] == directives

    # For pure requests we expect to resolve eou before apply
    assert response['meta'] == []

    event = VoiceInputEvent.from_utterance('включи видео', end_of_utterance=True)
    response = handle_event(
        event,
        ensure_purity=True,
        app_info=app_info,
        device_state=device_state,
    )
    # we ignore the contents of the payload.callback.sample.annotation_bag
    assert response['directives'][0]['payload']['callback']['sample']['annotations_bag']
    response['directives'][0]['payload']['callback']['sample']['annotations_bag'] = {}

    # the only diff in the expected response with previous case is in the end_of_utterance field of event
    directives[0]['payload']['callback']['event']['end_of_utterance'] = True
    assert not mocked_app.universal_callback.called
    assert response['features']['form_info']['intent'] == 'personal_assistant.scenarios.video_play'
    assert response['directives'] == directives


def test_apply_callback(vins_app):
    event = ServerActionEvent(
        name='apply_request',
        payload={
            'form_update': {
                'name': 'personal_assistant.scenarios.video_play',
                'set_new_form': False,
                'slots': [{
                    'name': 'search_text',
                    'optional': True,
                    'source_text': None,
                    'type': 'string',
                    'value': None
                }, {
                    'name': 'action',
                    'optional': True,
                    'source_text': 'включи',
                    'type': 'video_action',
                    'value': 'play'
                }, {
                    'name': 'content_provider',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_provider',
                    'value': None
                }, {
                    'name': 'content_type',
                    'optional': True,
                    'source_text': 'видео',
                    'type': 'video_content_type',
                    'value': 'video'
                }, {
                    'name': 'film_genre',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_film_genre',
                    'value': None
                }, {
                    'name': 'country',
                    'optional': True,
                    'source_text': None,
                    'type': 'geo_adjective',
                    'value': None
                }, {
                    'name': 'free',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_free',
                    'value': None
                }, {
                    'name': 'new',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_new',
                    'value': None
                }, {
                    'name': 'release_date',
                    'optional': True,
                    'source_text': None,
                    'type': 'year_adjective',
                    'value': None
                }, {
                    'name': 'top',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_top',
                    'value': None
                }, {
                    'name': 'season',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_season',
                    'value': None
                }, {
                    'name': 'episode',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_episode',
                    'value': None
                }, {
                    'name': 'video_result',
                    'optional': True,
                    'source_text': None,
                    'type': 'video_result',
                    'value': None
                }, {
                    'name': 'browser_video_gallery',
                    'optional': True,
                    'source_text': None,
                    'type': 'browser_video_gallery',
                    'value': None
                }],
            },
            'callback': {
                'arguments': {},
                'event': {
                    'asr_result': [
                        {
                            'confidence': 1.0,
                            'utterance': 'включи видео',
                            'words': [
                                {'confidence': 1.0, 'value': 'включи'},
                                {'confidence': 1.0, 'value': 'видео'}
                            ]
                        }
                    ],
                    'end_of_utterance': True,
                    'payload': {},
                    'type': 'voice_input'
                },
                'name': 'universal_callback',
                'sample': {
                    'annotations_bag': {},
                    'tags': ['O', 'O'],
                    'tokens': [
                        'включи',
                        'видео'
                    ],
                    'utterance': {
                        'input_source': 'voice',
                        'text': 'включи видео',
                    },
                    'weight': 1.0,
                }
            }
        }
    )
    device_state = {'is_tv_plugged_in': True}
    app_info = AppInfo(app_id='ru.yandex.quasar')

    def response_callback(request, context):
        request = json.loads(request.body)
        assert request['meta']['utterance'] == 'включи видео'
        assert request['form'] == event.payload['form_update']

        return request

    with form_handling_mock(response_callback):
        req_info = vins_app.prepare_req_info(
            event=event,
            uuid=str(gen_uuid()),
            ensure_purity=False,
            app_info=app_info,
            device_state=device_state,
            session=None,
        )
        vins_app._vins_app.handle_request(req_info)
        history = vins_app._vins_app.load_or_create_session(req_info).dialog_history.last(3)
        assert len(history) == 1
        assert history[0].form.name == 'personal_assistant.scenarios.video_play'


def test_apply_server_action(handle_event):
    event = ServerActionEvent(
        name='apply_request',
        payload={
            'callback': {
                'name': 'universal_callback',
                'arguments': {
                    'session_state': {},
                    'set_new_form': True,
                    'form_update': {
                        'name': 'personal_assistant.scenarios.taxi_new_status',
                        'slots': []
                    },
                    'resubmit': True
                },
                'event': {
                    'name': 'update_form',
                    'payload':
                        {
                            'session_state':
                                {
                                },
                            'set_new_form': True,
                            'form_update':
                                {
                                    'name': 'personal_assistant.scenarios.taxi_new_status',
                                    'slots':
                                        [
                                        ]
                                },
                            'resubmit': True
                        },
                    'type': 'server_action'
                }
            },
            'form_update': {
                'name': 'personal_assistant.scenarios.taxi_new_status',
                'set_new_form': False,
                'slots': [
                    {
                        'name': 'status',
                        'optional': True,
                        'type': 'string',
                        'value': None,
                        'source_text': None
                    },
                    {
                        'name': 'order_data',
                        'optional': True,
                        'type': 'order_data',
                        'value': None,
                        'source_text': None
                    },
                    {
                        'name': 'location_from',
                        'optional': True,
                        'type': 'geo',
                        'value': None,
                        'source_text': None
                    },
                    {
                        'name': 'location_to',
                        'optional': True,
                        'type': 'geo',
                        'value': None,
                        'source_text': None
                    },
                    {
                        'name': 'taxi_uid',
                        'optional': True,
                        'type': 'string',
                        'value': None,
                        'source_text': None
                    },
                    {
                        'name': 'map_url',
                        'optional': True,
                        'type': 'string',
                        'value': None,
                        'source_text': None
                    }
                ]
            }
        }
    )
    app_info = AppInfo(app_id='com.yandex.vins.tests')

    def response_callback(request, context):
        request = json.loads(request.body)
        return request

    with form_handling_mock(response_callback):
        handle_event(event, ensure_purity=False, app_info=app_info, device_state={})


def test_apply_nlg_callback(handle_event):
    app_info = AppInfo(app_id='com.yandex.vins.tests')

    def response_callback(request, context):
        request = json.loads(request.body)
        return request

    with form_handling_mock(response_callback):
        handle_event(
            VoiceInputEvent.from_utterance('запомни адрес', end_of_utterance=True),
            ensure_purity=True, app_info=app_info, device_state={}
        )
        response = handle_event(
            VoiceInputEvent.from_utterance('работы', end_of_utterance=True),
            ensure_purity=True, app_info=app_info, device_state={}
        )
        handle_event(
            ServerActionEvent(name='apply_request', payload=response['directives'][0]['payload']),
            ensure_purity=False, app_info=app_info, device_state={}
        )


def test_apply_always_with_eou(handle_event):
    device_state = {'is_tv_plugged_in': True}
    app_info = AppInfo(app_id='ru.yandex.quasar')

    def response_callback(request, context):
        request = json.loads(request.body)
        return request

    with form_handling_mock(response_callback):
        response = handle_event(
            VoiceInputEvent.from_utterance('включи видео', end_of_utterance=False),
            ensure_purity=True, app_info=app_info, device_state=device_state
        )
        response = handle_event(
            ServerActionEvent(name='apply_request', payload=response['directives'][0]['payload']),
            ensure_purity=False, app_info=app_info, device_state=device_state
        )
        assert {
            "error_type": "eou_expected",
            "type": "error"
        } not in response['meta']


def test_run_apply_session(vins_app):
    run_event = VoiceInputEvent.from_utterance('вызови такси домой', end_of_utterance=True)
    device_state = {'is_tv_plugged_in': True}
    app_info = AppInfo(app_id='ru.yandex.quasar')
    uuid = str(gen_uuid())
    session = Session(app_id=app_info.app_id, uuid=uuid)
    req_info = vins_app.prepare_req_info(
        event=run_event,
        uuid=uuid,
        ensure_purity=True,
        app_info=app_info,
        device_state=device_state,
        session=session,
    )
    response = vins_app._vins_app.handle_request(req_info)
    apply_event = ServerActionEvent(
        name='apply_request',
        payload=response.directives[0].payload
    )

    def response_callback(request, context):
        request = json.loads(request.body)

        for slot in request['form']['slots']:
            if slot['name'] == 'where_to':
                assert slot['value'] == 'домой'
                slot['value'] = 'в Дубровку'

        return request

    with form_handling_mock(response_callback):
        req_info = vins_app.prepare_req_info(
            event=apply_event,
            uuid=uuid,
            ensure_purity=False,
            app_info=app_info,
            device_state=device_state,
            session=session,
        )
        vins_app._vins_app.handle_request(req_info)
        history = vins_app._vins_app.load_or_create_session(req_info).dialog_history.last(3)

        # One turn for request, another - for the answer
        assert len(history) == 2
        assert history[0].form.name == 'personal_assistant.scenarios.taxi_new_order'
        assert history[0].form.get_slot_by_name('where_to').value == 'в Дубровку'
        assert history[1].form.name == 'personal_assistant.scenarios.taxi_new_order'
        assert history[1].form.get_slot_by_name('where_to').value == 'домой'


# todo(g-kostin): remove this test after all external skill sessions in tabs are Dialogovo only
def test_apply_external_button(handle_event):
    event = ServerActionEvent.from_dict({
        "name": "apply_request",
        "end_of_utterance": True,
        "payload":
            {
                "callback":
                    {
                        "name": "on_external_button",
                        "arguments":
                            {
                                "button_data": "{\"text\": \"Про острова\"}",
                                "request_id": "b277aa11-2df8-45d7-975d-ffffffff4b30"
                            },
                        "event":
                            {
                                "name": "on_external_button",
                                "payload":
                                    {
                                        "button_data": "{\"text\": \"Про острова\"}",
                                        "request_id": "b277aa11-2df8-45d7-975d-ffffffff4b30"
                                    },
                                "type": "server_action"
                            }
                    },
                "form_update":
                    {
                        "name": "personal_assistant.scenarios.external_skill__continue",
                        "set_new_form": False,
                        "slots":
                            [
                                {
                                    "name": "skill_id",
                                    "optional": True,
                                    "type": "skill",
                                    "value": "bd168a52-c2eb-43be-a3df-ffffffffd02b",
                                    "source_text": None
                                },
                                {
                                    "name": "skill_info",
                                    "optional": True,
                                    "type": "skill_info",
                                    "value":
                                        {
                                            "name": "Сказки",
                                            "developer_mode": False,
                                            "internal": False,
                                            "voice": "shitova.us",
                                            "zora": True
                                        },
                                    "source_text": None
                                },
                                {
                                    "name": "session",
                                    "optional": False,
                                    "type": "session",
                                    "value":
                                        {
                                            "seq": 1,
                                            "id": "4cc7f2bd-d7f5398-3f75a920-ffffffff"
                                        },
                                    "source_text": None
                                },
                                {
                                    "name": "request",
                                    "optional": True,
                                    "type": "string",
                                    "value": "а",
                                    "source_text": None
                                },
                                {
                                    "name": "vins_markup",
                                    "optional": True,
                                    "type": "vins_markup",
                                    "value":
                                        {
                                        },
                                    "source_text": None
                                },
                                {
                                    "name": "response",
                                    "optional": True,
                                    "type": "response",
                                    "value":
                                        {
                                            "voice": "Такой сказки у меня пока нет. Но если хотите, могу поставить про острова или сказка гарика burito. Или вообще — сказку-сюрприз.",  # noqa
                                            "text": "Такой сказки у меня пока нет. Но если хотите, могу поставить про острова или сказка гарика burito. Или вообще — сказку-сюрприз."    # noqa
                                        },
                                    "source_text": None
                                }
                            ]
                    }
            },
        "type": "server_action"
    })

    app_info = AppInfo(app_id='com.yandex.vins.tests')

    def response_callback(request, context):
        request = json.loads(request.body)
        return request

    with form_handling_mock(response_callback):
        handle_event(event, ensure_purity=False, app_info=app_info, device_state={})


def test_forbidden_intents(vins_app):
    utterance = 'погода в москве'
    req_info_kwargs = deepcopy(vins_app.get_default_reqinfo_kwargs())
    req_info_kwargs['app_info'] = AppInfo(
        app_id='ru.yandex.quasar.test',
        app_version='0.0.1',
        os_version='1',
        platform='unknown',
    )
    req_info = ReqInfo(
        uuid=str(gen_uuid()),
        utterance=Utterance(utterance),
        client_time=utcnow(),
        event=VoiceInputEvent.from_utterance(utterance, end_of_utterance=False),
        **req_info_kwargs
    )
    with check_meta_mock(dict(
            experiments={
                'forbidden_intents':
                    'personal_assistant.scenarios.quasar.select_video_from_gallery_by_text,'
                    'personal_assistant.scenarios.external_skill__activate_only,'
                    'personal_assistant.scenarios.quasar.authorize_video_provider,'
                    'personal_assistant.scenarios.quasar.goto_video_screen,'
                    'personal_assistant.handcrafted.goto_blogger_external_skill,'
                    'personal_assistant.scenarios.music_play,'
                    'personal_assistant.scenarios.quasar.select_video_from_gallery,'
                    'personal_assistant.scenarios.video_play,'
                    'personal_assistant.scenarios.quasar.payment_confirmed,'
                    'personal_assistant.scenarios.video_play_entity,'
                    'personal_assistant.scenarios.external_skill,'
                    'personal_assistant.scenarios.quasar.open_current_video',
            }
    )):
        vins_app.handle_reqinfo(req_info=req_info)


@freeze_time('2016-10-26 21:13')
def test_overridden_analytics_info(f, s):
    with current_weather_api_mock(temperature=-1, condition='снег'):
        assert f('расскажи погоду') == 'Сейчас в Москве -1, снег.'
    with gc_mock(api_version=2, url=GC_CONFIG['url'], response='б'):
        f('а')
    spb = _city_cases('Санкт-Петербурге')
    with current_weather_api_mock(
            temperature=-3,
            condition='как всегда ветренно',
            location=spb,
    ):
        response = s('а в питере?')

        assert response.cards[0].text == 'В настоящее время в Санкт-Петербурге -3, как всегда ветренно.'
        assert response.analytics_info_meta.intent == 'personal_assistant.scenarios.get_weather__ellipsis'
