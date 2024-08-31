import re

import alice.tests.library.auth as auth
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


POSTROLL_PHRASE_AMBIENT = 'Кстати, я могу включить вам звуки природы. Только скажите - и услышите шум моря перед сном.'
POSTROLL_PHRASE_AMBIENT_BUTTON = 'Я точно знаю, что нервные клетки потихоньку восстанавливаются, если включать им звук природы. Включить сейчас?'
POSTROLL_PHRASE_MORNING_SHOW = 'Кстати, если бы каждое утро слушали моё шоу, вы бы уже знали про погоду.'
POSTROLL_PHRASE_MUSIC = 'Включить вам джаз?'
POSTROLL_PHRASE_RADIO_BUTTON = 'Кстати, я всегда могу включить вам радио Максимум. Включить?'
POSTROLL_PHRASE_WEATHER = 'Кстати, я знаю, какая погода завтра. Только спросите - и я расскажу.'

EXPERIMENTS_ENABLE_TEXT_ANSWER = [
    'mm_proactivity_debug_text_response',
    'skillrec_debug_text_response',
]

EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD = [
    'mm_proactivity_request_delta_threshold=0',
    'mm_proactivity_time_delta_threshold=0',
]

NOTIFICATION_SOUND_REGEX = re.compile(r'<speaker audio="postroll_notification_sound.opus">')


class ActionFields:
    def __init__(self, postroll_id, action_type):
        self.postroll_id = postroll_id,
        self.action_type = action_type      # 0 = View, 1 = Click, 4 = Stop, 5 = Declain

    def __hash__(self):
        return hash((self.postroll_id, self.action_type))

    def __eq__(self, other):
        return self.postroll_id == other.postroll_id and self.action_type == other.action_type


@pytest.mark.experiments(
    'skillrec_evo',
    'skillrec_time_delta=0',
    'skillrec_request_delta=0',
    'skillrec_see_shown',
    'skillrec_disable_all_items',
    'mm_proactivity_disable_memento',
    'mm_proactivity_dont_save_to_memento',
)
class _TestProactivityService(object):

    owners = ('karina-usm', 'jan-fazli',)

    def _response_details(self, response):
        # TODO(jan-fazli) Also print everything else used in success conditions (slots, device state, etc)
        return f'Winner intent: {response.intent}. Note that it may NOT be the cause ' \
            'of success condition failure, since intent and frame name are not always identical!'

    def _assert_no_postroll(self, response, postroll_text=None):
        proactivity_info = response.megamind_modifiers_info.proactivity
        assert not proactivity_info.get('appended')
        assert not proactivity_info.get('item_id')
        assert not proactivity_info.get('item_info')
        assert not proactivity_info.get('disabled_in_app')
        if postroll_text:
            assert not response.text.endswith(postroll_text)

    def _assert_proactivity_response(self, response, postroll_id, postroll_intent,
                                     postroll_voice=None, postroll_text=None, with_notification_sound=False, is_marketing=None):
        proactivity_info = response.megamind_modifiers_info.proactivity
        assert proactivity_info['appended']
        assert proactivity_info['from_skill_rec']
        assert proactivity_info['item_id'] == postroll_id
        assert proactivity_info['item_info'] == postroll_intent
        assert not proactivity_info.get('disabled_in_app')
        if postroll_voice:
            assert response.has_voice_response()
            assert response.output_speech_text.endswith(postroll_voice)
        if postroll_text:
            card_num = len(response.cards)
            assert card_num > 0
            assert response.cards[card_num - 1].text.endswith(postroll_text)
        if with_notification_sound:
            re.search(NOTIFICATION_SOUND_REGEX, response.output_speech_text)
        if is_marketing is not None:
            assert proactivity_info.get('is_marketing_postroll', False) == is_marketing

    def _assert_proactivity_click(self, response, has_click, postroll_id=None, base_id=None, item_info=None):
        proactivity_info = response.megamind_modifiers_info.proactivity
        postroll_click_ids = proactivity_info.get('postroll_click_ids', [])
        postroll_clicks = proactivity_info.get('postroll_clicks', [])
        if has_click:
            assert len(postroll_click_ids) == 1, self._response_details(response)
            if postroll_id is not None:
                assert postroll_click_ids[0] == postroll_id, self._response_details(response)
            if len(postroll_clicks) > 0:
                if postroll_id is not None:
                    assert postroll_clicks[0]['item_id'] == postroll_id, self._response_details(response)
                if base_id is not None:
                    assert postroll_clicks[0]['base_id'] == base_id, self._response_details(response)
                if item_info is not None:
                    assert postroll_clicks[0]['item_info'] == item_info, self._response_details(response)
        else:
            assert len(postroll_click_ids) == 0

    def _assert_proactivity_actions(self, actions, expected_action_set):
        action_set = set([])
        for action in actions:
            action_set.add(ActionFields(action.get('ToId'), action.get('ActionType')))
        assert(action_set == expected_action_set)


@pytest.mark.voice
@pytest.mark.experiments('skillrec_enable_item=evo__weather', *EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD)
class TestProactivity(_TestProactivityService):

    @pytest.mark.parametrize('surface', [surface.station])
    def test_postroll_enabled(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__weather',
            postroll_intent=intent.GetWeather,
            postroll_voice=POSTROLL_PHRASE_WEATHER,
            is_marketing=False
        )

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('skillrec_must_have_all_request_fields')
    def test_postroll_has_all_request_fields(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__weather',
            postroll_intent=intent.GetWeather,
            postroll_voice=POSTROLL_PHRASE_WEATHER,
            is_marketing=False
        )

    @pytest.mark.version(megamind=249)
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.experiments('mm_proactivity_enable_notification_sound')
    def test_postroll_add_sound(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__weather',
            postroll_intent=intent.GetWeather,
            postroll_voice=POSTROLL_PHRASE_WEATHER,
            with_notification_sound=True,
            is_marketing=False
        )

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.experiments('skillrec_disable')
    def test_postroll_disabled(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert response.has_voice_response()
        assert not response.output_speech_text.endswith(POSTROLL_PHRASE_WEATHER)

        proactivity_info = response.megamind_modifiers_info.proactivity
        assert not proactivity_info.get('appended')
        assert not proactivity_info.get('item_id')
        assert not proactivity_info.get('item_info')
        assert not proactivity_info.get('disabled_in_app')

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_postroll_unsupportedd_device(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert response.has_voice_response()
        assert not response.output_speech_text.endswith(POSTROLL_PHRASE_WEATHER)

        proactivity_info = response.megamind_modifiers_info.proactivity
        assert not proactivity_info.get('appended')
        assert not proactivity_info.get('item_id')
        assert not proactivity_info.get('item_info')
        assert not proactivity_info.get('disabled_in_app')


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('skillrec_enable_item=evo__weather')
class TestProactivityTimeDeltaThreshold(_TestProactivityService):

    @pytest.mark.experiments('mm_proactivity_time_delta_threshold=50000000', 'mm_proactivity_request_delta_threshold=0')
    def test_disabled_by_timedelta_threshold(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert response.has_voice_response()
        assert not response.output_speech_text.endswith(POSTROLL_PHRASE_WEATHER)

        proactivity_info = response.megamind_modifiers_info.proactivity
        assert not proactivity_info.get('appended')
        assert not proactivity_info.get('item_id')
        assert not proactivity_info.get('item_info')
        assert not proactivity_info.get('disabled_in_app')

    @pytest.mark.experiments('mm_proactivity_time_delta_threshold=0', 'mm_proactivity_request_delta_threshold=50000000')
    def test_disabled_by_request_threshold(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert response.has_voice_response()
        assert not response.output_speech_text.endswith(POSTROLL_PHRASE_WEATHER)

        proactivity_info = response.megamind_modifiers_info.proactivity
        assert not proactivity_info.get('appended')
        assert not proactivity_info.get('item_id')
        assert not proactivity_info.get('item_info')
        assert not proactivity_info.get('disabled_in_app')


@pytest.mark.text
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('skillrec_enable_item=evo__weather', *EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD)
class TestProactivityTextResponse(_TestProactivityService):

    def test_disabled_for_text_event_response(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert not response.has_voice_response()
        self._assert_no_postroll(response, POSTROLL_PHRASE_WEATHER)

    @pytest.mark.experiments(*EXPERIMENTS_ENABLE_TEXT_ANSWER)
    def test_enabled_for_text_response(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert not response.has_voice_response()
        self._assert_no_postroll(response)

    @pytest.mark.experiments('mm_proactivity_enable_on_any_event', *EXPERIMENTS_ENABLE_TEXT_ANSWER)
    def test_enabled_for_text_event_and_response(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        assert not response.has_voice_response()
        self._assert_proactivity_response(
            response,
            postroll_id='evo__weather',
            postroll_intent=intent.GetWeather,
            postroll_text=POSTROLL_PHRASE_WEATHER,
            is_marketing=False
        )


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(*EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD)
class TestProactivityClicks(_TestProactivityService):

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient')
    def test_postroll_click_ambient_sound(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__ambient',
            postroll_intent=intent.MusicAmbientSound,
            postroll_voice=POSTROLL_PHRASE_AMBIENT,
            is_marketing=False
        )

        response = alice('включи звуки природы')
        assert response.intent == intent.MusicAmbientSound
        self._assert_proactivity_click(response, has_click=True, postroll_id='evo__ambient')

    @pytest.mark.experiments('skillrec_enable_item=evo__weather_marketing')
    def test_postroll_click_vins(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__weather_marketing',
            postroll_intent=intent.GetWeather,
            postroll_voice=POSTROLL_PHRASE_WEATHER,
            is_marketing=True
        )

        response = alice('сколько времени')
        assert response.intent == intent.GetTime
        assert response.has_voice_response()
        self._assert_proactivity_click(response, has_click=False)

        response = alice('какая сегодня погода')
        assert response.intent == intent.GetWeather
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__weather_marketing',
                                       base_id='weather',
                                       item_info='personal_assistant.scenarios.get_weather')

    @pytest.mark.experiments('skillrec_enable_item=evo__morning_show_weather')
    def test_morning_show_click(self, alice):
        response = alice('какая сегодня погода')
        assert response.intent == intent.GetWeather
        self._assert_proactivity_response(
            response,
            postroll_id='evo__morning_show_weather',
            postroll_intent=intent.MorningShow,
            postroll_voice=POSTROLL_PHRASE_MORNING_SHOW,
            is_marketing=False
        )

        response = alice('включи утренее шоу')
        assert response.scenario == scenario.AliceShow
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__morning_show_weather',
                                       base_id='alice_show',
                                       item_info='alice.vinsless.music.morning_show')

    @pytest.mark.experiments('skillrec_enable_item=evo__music_genres')
    def test_music_click(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__music_genres',
            postroll_intent=intent.MusicPlay,
            postroll_voice=POSTROLL_PHRASE_MUSIC,
            is_marketing=False
        )

        response = alice('включи')
        assert response.intent == intent.MusicPlay
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__music_genres',
                                       base_id='music_with_button__genres',
                                       item_info='personal_assistant.scenarios.music_play')

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient_button')
    def test_ambient_button_click(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__ambient_button',
            postroll_intent=intent.MusicAmbientSound,
            postroll_voice=POSTROLL_PHRASE_AMBIENT_BUTTON,
            is_marketing=False
        )

        response = alice('включи')
        assert response.intent == intent.MusicPlay
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__ambient_button',
                                       base_id='ambient_with_button',
                                       item_info='personal_assistant.scenarios.music_ambient_sound')

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient_button')
    def test_ambient_button_indirect_click(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__ambient_button',
            postroll_intent=intent.MusicAmbientSound,
            postroll_voice=POSTROLL_PHRASE_AMBIENT_BUTTON,
            is_marketing=False
        )

        response = alice('включи звуки природы')
        assert response.intent == intent.MusicAmbientSound
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__ambient_button',
                                       base_id='ambient_with_button',
                                       item_info='personal_assistant.scenarios.music_ambient_sound')

    @pytest.mark.experiments('skillrec_enable_item=evo__radio_button', 'postroll_radio_with_button')
    def test_radio_click(self, alice):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id='evo__radio_button',
            postroll_intent=intent.RadioPlay,
            postroll_voice=POSTROLL_PHRASE_RADIO_BUTTON,
            is_marketing=False
        )

        response = alice('включи')
        assert response.intent == intent.RadioPlay
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__radio_button',
                                       base_id='radio_with_button',
                                       item_info='personal_assistant.scenarios.radio_play')

        response = alice('включи радио максимум')
        assert response.intent == intent.RadioPlay
        self._assert_proactivity_click(response,
                                       has_click=True,
                                       postroll_id='evo__radio_button',
                                       base_id='radio_with_button',
                                       item_info='personal_assistant.scenarios.radio_play')


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments('proactivity_log_storage', *EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD)
class TestProactivityLogStorage(_TestProactivityService):

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient_button')
    def test_click_logging(self, alice):
        response = alice('привет')
        expected_action_set = set([ActionFields('evo__ambient_button', 0)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('включи')
        expected_action_set = set([ActionFields('evo__ambient_button', 1)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('включи звуки природы')
        expected_action_set = set([ActionFields('evo__ambient_button', 1)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient_button')
    def test_declain_logging(self, alice):
        response = alice('привет')
        expected_action_set = set([ActionFields('evo__ambient_button', 0)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('нет спасибо')
        expected_action_set = set([ActionFields('evo__ambient_button', 5)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

    @pytest.mark.experiments('skillrec_enable_item=evo__ambient')
    def test_stop_logging(self, alice):
        response = alice('привет')
        expected_action_set = set([ActionFields('evo__ambient', 0)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('иди нахуй')
        expected_action_set = set([ActionFields('evo__ambient', 4)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

    @pytest.mark.experiments('skillrec_enable_item=evo__weather', 'skillrec_enable_item=evo__morning_show_weather')
    def test_multiple_logging(self, alice):
        response = alice('привет')
        expected_action_set = set([ActionFields('evo__weather', 0)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('какая погода сегодня')
        expected_action_set = set([ActionFields('evo__weather', 1), ActionFields('evo__morning_show_weather', 0)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )

        response = alice('включи утреннее шоу')
        expected_action_set = set([ActionFields('evo__morning_show_weather', 1)])
        self._assert_proactivity_actions(
            response.proactivity_log_storage_actions,
            expected_action_set
        )


@pytest.mark.text
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.experiments(
    'skillrec_debug_ignore_conditions',
    'mm_proactivity_debug_text_response',
    'mm_proactivity_enable_on_any_event',
    *EXPERIMENTS_DISABLE_TIMEDELTA_THRESHOLD,
)
class TestProactivityAllClicks(_TestProactivityService):

    # default utterance for button
    def check_click(self, alice, id=None, info=None, phrase=None, utterance='включи'):
        response = alice('привет')
        assert response.intent == intent.Hello
        self._assert_proactivity_response(
            response,
            postroll_id=id,
            postroll_intent=info,
            postroll_text=phrase
        )

        response = alice(utterance)

        self._assert_proactivity_click(response, has_click=True, postroll_id=id)

    # NOTICE !!!
    # This is an example for tests created by dj/services/alisa_skils/tools/evo_generator
    # If you change this test, you MUST also change the generator tool (MakeTest function)
    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.experiments('skillrec_enable_item=evo__ambient_button')
    def test_postroll_click_example(self, alice):
        self.check_click(
            alice,
            id='evo__ambient_button',
            info=intent.MusicAmbientSound,
            phrase=POSTROLL_PHRASE_AMBIENT_BUTTON,
            utterance='включи звуки природы')

    # NOTICE !!! DO NOT ADD TESTS AFTER THIS
    # Test generator adds tests for clicks here
    # Add new hand-made tests above instead
