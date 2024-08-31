import base64

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


DEFAULT_GC_TRIGGER_TEXT = 'ты китик'


def get_proactivity_action_name(response):
    return response.scenario_analytics_info.object('gc_response_info').get('proactivity_info', dict()).get('action_name')


def get_proactivity_entity_key(response):
    return response.scenario_analytics_info.object('gc_response_info').get('proactivity_info', dict()).get('entity_key')


def get_disscussion_info_entity_key(response):
    return response.scenario_analytics_info.object('gc_response_info').get('discussion_info', dict()).get('entity_key')


def get_gives_negative_feedback(response):
    discussion_info = response.scenario_analytics_info.object('gc_response_info').get('discussion_info', dict())
    return discussion_info.get('gives_negative_feedback', False)


def get_source(response):
    return response.scenario_analytics_info.object('gc_response_info').get('source')


def get_gc_intent(response):
    return response.scenario_analytics_info.object('gc_response_info').get('gc_intent')


def get_original_intent(response):
    return response.scenario_analytics_info.object('gc_response_info').get('original_intent')


def get_is_aggregated(response):
    return response.scenario_analytics_info.object('gc_response_info').get('is_aggregated_request')


def get_facts_crosspromo_entity_key(response):
    return response.scenario_analytics_info.object('gc_response_info').get('facts_crosspromo_info', dict()).get('entity_key')


@pytest.mark.experiments('mm_gc_protocol_disable', 'hw_gc_protocol_disable', 'mm_preclassifier_thresholds=Vins:-40.0')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestVins(object):

    owners = ('nzinov',)

    def test_gc(self, alice):
        response = alice('я сегодня сходил в музей')
        assert response.intent == intent.GeneralConversation


@pytest.mark.parametrize('surface', [surface.searchapp, surface.launcher])
class TestProtocolFullSearchButton(object):

    owners = ('nzinov',)

    def _has_search_suggest(self, response):
        for suggest in response.suggests:
            if '🔍' in suggest.title:
                return True

        return False

    def test_gc(self, alice):
        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'dummy'
        assert self._has_search_suggest(response)


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestProtocolFull(object):

    owners = ('nzinov',)

    def _has_what_can_you_do_suggest(self, response):
        return response.suggests[-1].title.startswith('Что ты умеешь?')

    def _has_stop_suggest(self, response):
        return response.suggests[0].title.startswith('Хватит болтать')

    def _has_feedback_suggests(self, response):
        suggests = [s.title for s in response.suggests]
        return suggests == ['Отлично', 'Нормально', 'Не очень']

    def test_gc(self, alice):
        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'dummy'
        assert self._has_what_can_you_do_suggest(response)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICERELEASE-3463#622b051868e59f0a455e3a93')
    def test_ban(self, alice):
        response = alice('как ты относишься к путину')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversationDummy
        assert self._has_what_can_you_do_suggest(response)

    def test_microintent(self, alice):
        response = alice('как тебя зовут')
        assert response.intent == intent.WhatIsYourName

    def test_pure(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation
        assert self._has_stop_suggest(response)
        assert not self._has_what_can_you_do_suggest(response)

        response = alice('какая сейчас погода')
        assert response.scenario == scenario.Weather

        response = alice('что ты думаешь о путине')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.GeneralConversationDummy
        assert self._has_stop_suggest(response)
        assert self._has_what_can_you_do_suggest(response)

        response = alice('что ты думаешь о котиках?')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert self._has_stop_suggest(response)
        assert self._has_what_can_you_do_suggest(response)

        response = alice('хватит болтать')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.PureGeneralConversationDeactivation
        assert self._has_feedback_suggests(response)
        assert not self._has_what_can_you_do_suggest(response)

        response = alice('нормально')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.PureGeneralConversationFeedbackNeutral

        response = alice('какая сейчас погода')
        assert response.scenario == scenario.Weather


@pytest.mark.experiments('hw_gc_enable_aggregated_reply')
class TestProtocolFullAggregated(TestProtocolFull):

    owners = ('nzinov',)


@pytest.mark.experiments('hw_gc_enable_aggregated_reply_in_modal_mode')
class TestProtocolFullAggregatedModalMode(TestProtocolFull):

    owners = ('nzinov',)


@pytest.mark.experiments('hw_gc_debug_pure_gc_session_timeout=0')
@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestProtocolPureGcSessionTimeout(object):

    owners = ('nzinov',)

    def test_weather(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('какая сейчас погода')
        assert response.intent == intent.GetWeather

        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'dummy'

    def test_gc(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.intent == intent.GeneralConversation

    def test_activate(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation


@pytest.mark.version(general_conversation=58)
@pytest.mark.experiments('hw_gc_add_gif_to_answer', 'hw_gc_first_reply')
class TestProtocolGifAnswer(object):

    owners = ('alzaharov',)

    @staticmethod
    def _check_no_gif_answer(response):
        assert len(response.cards) == 1
        assert response.text_card
        assert not response.div_card

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_no_gif_general_gc(self, alice):
        response = alice('алиса мне очень хочется плакать плакать')
        self._check_no_gif_answer(response)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_no_gif_in_pure_gc(self, alice):
        alice('давай поболтаем')
        response = alice('я тебя люблю')
        self._check_no_gif_answer(response)

    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_no_gif_in_quasar(self, alice):
        alice('давай поболтаем')
        response = alice('я тебя люблю')
        self._check_no_gif_answer(response)


@pytest.mark.experiments('hw_gc_first_reply')
@pytest.mark.experiments('hw_gc_enable_modality_in_pure_gc')
class TestProtocolEmojiAnswer(object):

    owners = ('alzaharov',)

    @pytest.mark.parametrize('surface', [surface.searchapp])
    def test_emoji_searchapp(self, alice):
        alice('давай поболтаем')
        response = alice('я тебя люблю')
        assert '❤️' in response.text

    @pytest.mark.parametrize('surface', [surface.station, surface.loudspeaker])
    def test_emoji_unsupported(self, alice):
        alice('давай поболтаем')
        response = alice('я тебя люблю')
        assert '❤️' not in response.text


@pytest.mark.parametrize('surface', [surface.station_pro])
@pytest.mark.experiments('hw_gc_first_reply')
class TestProtocolAnswerWithLedImage(object):

    owners = ('alzaharov',)

    @staticmethod
    def _check_led_image_answer(response, gif_name):
        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.ForceDisplayCardsDirective
        assert response.directives[1].name == directives.names.DrawLedScreenDirective

        seq = response.directives[1].payload.animation_sequence
        assert len(seq) == 1
        assert gif_name in seq[0].frontal_led_image

    def test_supported_neutral(self, alice):
        response = alice('что ты думаешь о котиках')
        self._check_led_image_answer(response, 'emotions/neutral.gif')

    def test_supported_pure_neutral(self, alice):
        alice('давай поболтаем')
        response = alice('как дела')
        self._check_led_image_answer(response, 'emotions/neutral.gif')

    def test_supported_emotional(self, alice):
        alice('давай поболтаем')
        response = alice('я тебя люблю')
        self._check_led_image_answer(response, 'emotions/love.gif')

    @pytest.mark.supported_features(led_display=None)
    def test_unsupported_neutral(self, alice):
        response = alice('что ты думаешь о котиках')
        assert not response.directives


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolIndexProactivity(object):

    owners = ('nzinov',)
    experiments = [
        'hw_gc_disable_movie_discussions_by_default',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
        'hw_gc_proactivity',
        'hw_gc_reply_ProactivityBoost=10000000',
        'hw_gc_force_proactivity_soft',
        'hw_gc_reply_EntityBoost=10000000',
        'hw_gc_entity_index',
        'hw_gc_force_entity_soft',
    ]

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    def test_movie_discuss(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_entity_discussion_question_suggest')
    def test_movie_discuss_question_suggest(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'
        question_suggest = response.suggests[0].title
        assert question_suggest.endswith('?')

        for _ in range(5):
            response = alice(question_suggest)
            assert response.scenario == scenario.GeneralConversation
            assert response.intent == intent.GeneralConversation
            assert get_source(response) == 'movie_specific'
            assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'
            question_suggest = response.suggests[0].title
            assert question_suggest.endswith('?')

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_movie_open_suggest_prob=1.0')
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('play_request', [
        'включи его',
        'включи этот фильм',
        'включи фильм Джентльмены',
    ])
    def test_movie_discuss_movie_open(self, alice, play_request):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('Джентльмены')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:1143242'
        assert response.suggests[0].title == 'Включи фильм «Джентльмены»'

        response = alice(play_request)
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name == 'Джентльмены'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific')
    def test_movie_discuss_specific(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'
        entity_key = response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key']
        assert entity_key.startswith('movie:')

        response = alice('я смотрел этот фильм. норм. а тебе как?')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss')
    def test_modal_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific')
    def test_modal_movie_discuss_specific(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        entity_key = response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key']
        assert entity_key.startswith('movie:')

        response = alice('норм фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_entity_discussion_question_suggest')
    def test_modal_movie_discuss_question_suggest(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'
        question_suggest = response.suggests[1].title
        assert question_suggest.endswith('?')

        response = alice(question_suggest)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific', 'hw_gc_entity_discussion_question_suggest')
    def test_modal_movie_discuss_specific_question_suggest(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        entity_key = response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key']
        assert entity_key.startswith('movie:')

        response = alice('я смотрел этот фильм. норм. а тебе как?')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key
        question_suggest = response.suggests[1].title
        assert question_suggest.endswith('?')

        response = alice(question_suggest)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_entity_discussion_question_prob=1.0', 'hw_gc_entity_discussion_question_alice')
    def test_movie_discuss_question_reply(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_original_intent(response) == intent.MovieDiscussFactQuestion
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == 'movie:714248'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific', 'hw_gc_entity_discussion_question_prob=1.0', 'hw_gc_entity_discussion_question_alice')
    def test_movie_discuss_specific_question_reply(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'
        entity_key = response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key']
        assert entity_key.startswith('movie:')

        response = alice('я смотрел этот фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_original_intent(response) == intent.MovieDiscussOpinionQuestion
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolIndexProactivityWithClassifier(object):

    owners = ('nzinov',)
    experiments = [
        'hw_gc_disable_movie_discussions_by_default',
        'hw_gc_proactivity',
        'hw_gc_reply_ProactivityBoost=100',
        'hw_gc_force_proactivity_soft',
        'hw_gc_proactivity_movie_discuss',
    ]

    @pytest.mark.experiments('hw_gc_proactivity_forbidden_dialog_turn_count_less=2')
    def test_movie_discuss_count(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

    @pytest.mark.experiments('hw_gc_proactivity_forbidden_dialog_turn_count_less=2', 'hw_gc_proactivity_forbidden_prev_scenario_timeout=0')
    def test_movie_discuss_timeout(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) != 'proactivity'


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolIndexProactivityEntityBoost(object):

    owners = ('nzinov',)
    experiments = [
        'hw_gc_disable_movie_discussions_by_default',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
        'hw_gc_proactivity',
        'hw_gc_reply_ProactivityBoost=100',
        'hw_gc_force_proactivity_soft',
        'hw_gc_entity_index',
    ]

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss', 'hw_gc_proactivity_movie_discuss_entity_boost=15')
    def test_movie_discuss(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific', 'hw_gc_proactivity_movie_discuss_specific_wacthed_entity_boost=15')
    def test_movie_discuss_specific_yes(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'
        entity_key = get_proactivity_entity_key(response)
        assert entity_key is not None

        response = alice('да')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key

    @pytest.mark.experiments('hw_gc_proactivity_movie_discuss_specific', 'hw_gc_proactivity_movie_discuss_specific_not_wacthed_entity_boost=15')
    def test_movie_discuss_specific_no(self, alice):
        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'proactivity'
        entity_key = get_proactivity_entity_key(response)
        assert entity_key is not None

        response = alice('нет')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolFrameProactivity(object):

    owners = ('nzinov',)
    experiments = [
        'hw_gc_disable_movie_discussions_by_default',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
        'hw_gc_frame_proactivity',
        'hw_gc_reply_EntityBoost=10000000',
        'hw_gc_entity_index',
        'hw_gc_force_entity_soft',
        'mm_postclassifier_gc_force_intents=alice.general_conversation.proactivity.alice_do;alice.general_conversation.proactivity.bored',
    ]

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss')
    def test_movie_discuss(self, alice):
        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss_specific')
    def test_movie_discuss_specific(self, alice):
        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('я смотрел этот фильм. норм. а тебе как?')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss')
    def test_modal_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) is None

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gcp_proactivity_movie_discuss_specific')
    def test_modal_movie_discuss_specific(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)
        assert entity_key.startswith('movie:')

        response = alice('норм фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_proactivity_action_name(response) is None
        assert get_disscussion_info_entity_key(response) == entity_key


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolLetsDiscussMovie(object):

    owners = ('nzinov',)
    experiments = [
        'hw_gc_disable_movie_discussions_by_default',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
        'hw_gc_lets_discuss_movie_frames',
        'hw_gc_reply_EntityBoost=100',
        'hw_gc_entity_index',
        'hw_gc_force_entity_soft',
        'mm_postclassifier_gc_force_intents=alice.general_conversation.lets_discuss_specific_movie;alice.general_conversation.lets_discuss_some_movie',
    ]

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7552')
    def test_lets_discuss_movie_general(self, alice):
        entity_key_matrix = 'movie:301'
        response = alice('давай обсудим фильм матрица')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response).endswith(intent.LetsDiscussSpecificMovie)
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('мне он понравился')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('о чем он')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('давай обсудим другой фильм')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_movie = get_disscussion_info_entity_key(response)
        assert entity_key_some_movie is not None
        assert entity_key_some_movie != entity_key_matrix

        response = alice('о чем он')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_some_movie

        response = alice('давай обсудим мультфильм')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_mult = get_disscussion_info_entity_key(response)
        assert entity_key_some_mult is not None
        assert entity_key_some_mult != entity_key_matrix
        assert entity_key_some_mult != entity_key_some_movie

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7552')
    @pytest.mark.experiments('hw_gc_entity_discussion_question_prob=1.0')
    def test_lets_discuss_movie_specific_question(self, alice):
        response = alice('давай обсудим фильм матрица')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSpecificMovie
        assert get_disscussion_info_entity_key(response) == 'movie:301'
        assert response.text[-1] == '?'

    def test_lets_discuss_movie_some_question(self, alice):
        response = alice('давай обсудим фильм')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_movie = get_disscussion_info_entity_key(response)
        assert entity_key_some_movie is not None

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-7552')
    @pytest.mark.experiments('hw_gc_force_entity_discussion_sentiment=negative')
    def test_lets_discuss_changed_movie(self, alice):
        response = alice('давай обсудим фильм Полицейский с Рублевки. Новогодний беспредел')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response).endswith(intent.LetsDiscussSpecificMovie)
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:1161783'
        assert get_gives_negative_feedback(response)

        # A movie without negative replies (it may change in the future)
        response = alice('давай обсудим фильм матрица')
        assert response.scenario == scenario.GeneralConversation
        assert get_original_intent(response).endswith(intent.LetsDiscussSpecificMovie)
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:301'
        assert not get_gives_negative_feedback(response)


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolFactsCrosspromo(object):

    owners = ('fadeich',)
    experiments = ['hw_facts_crosspromo_scenario_filter_disable', 'hw_facts_crosspromo_change_questions']

    def test_facts_crosspromo(self, alice):
        response = alice('давай поговорим о кошках')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_facts_crosspromo_entity_key(response) == 'koshki,kotov'

        response = alice('давай поговорим о собаках')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_facts_crosspromo_entity_key(response) is None

    @pytest.mark.experiments('hw_facts_crosspromo_timeout=0')
    def test_facts_crosspromo_timeout(self, alice):
        response = alice('давай поговорим о кошках')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_facts_crosspromo_entity_key(response) == 'koshki,kotov'

        response = alice('давай поговорим о собаках')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.GeneralConversation
        assert get_facts_crosspromo_entity_key(response) == 'sobake,sobake'


@pytest.mark.parametrize('surface', [surface.station])
class TestProtocolMovieDiscussionsByDefault(object):

    owners = ('nzinov',)

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gcp_proactivity_force_soft=alice.movie_discuss')
    def test_modal_frame_proactivity_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gcp_proactivity_force_soft=alice.movie_discuss')
    def test_modal_frame_proactivity_movie_discuss_i_dont_know(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('не знаю')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) is not None

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gcp_proactivity_force_soft=alice.movie_discuss_specific')
    @pytest.mark.parametrize('user_reaction', ['да', 'нет', 'не знаю'])
    def test_modal_frame_proactivity_movie_discuss_specific(self, alice, user_reaction):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) == 'alice.general_conversation.proactivity.bored'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)

        response = alice(user_reaction)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('мне скучно')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) != 'alice.general_conversation.proactivity.bored'

    @pytest.mark.experiments('hw_gc_reply_ProactivityBoost=100')
    def test_index_proactivity(self, alice):
        for _ in range(3):
            response = alice('ага')
            assert response.scenario == scenario.GeneralConversation
            assert response.intent == intent.GeneralConversation
            assert get_source(response) != 'proactivity'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments('hw_gc_reply_ProactivityBoost=10000000')
    def test_modal_index_proactivity(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments(
        'hw_gc_proactivity_force_soft=alice.movie_discuss',
        'hw_gc_reply_ProactivityBoost=10000000',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
    )
    def test_modal_index_proactivity_movie_discuss(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('песнь моря')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == 'movie:714248'

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments(
        'hw_gc_proactivity_force_soft=alice.movie_discuss',
        'hw_gc_reply_ProactivityBoost=10000000',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
    )
    def test_modal_frame_proactivity_movie_discuss_i_dont_know_1(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss'

        response = alice('не знаю')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_disscussion_info_entity_key(response) is not None

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    @pytest.mark.experiments(
        'hw_gc_proactivity_force_soft=alice.movie_discuss_specific',
        'hw_gc_reply_ProactivityBoost=10000000',
        'hw_gc_proactivity_forbidden_dialog_turn_count_less=0',
    )
    @pytest.mark.parametrize('user_reaction', ['да', 'нет', 'не знаю'])
    def test_modal_frame_proactivity_movie_discuss_specific_1(self, alice, user_reaction):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'proactivity'
        assert get_proactivity_action_name(response) == 'alice.movie_discuss_specific'
        entity_key = get_disscussion_info_entity_key(response)

        response = alice(user_reaction)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key

        response = alice('ага')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'

    def test_lets_discuss_movie_general(self, alice):
        response = alice('давай обсудим фильм матрица')
        assert response.scenario != scenario.GeneralConversation

    @pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICEINFRA-769')
    def test_modal_lets_discuss_movie_general(self, alice):
        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation

        entity_key_matrix = 'movie:301'
        response = alice('давай обсудим фильм матрица')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response).endswith(intent.LetsDiscussSpecificMovie)
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('о чем фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('интересно увидеть продолжение')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_matrix

        response = alice('давай обсудим другой фильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_movie = get_disscussion_info_entity_key(response)
        assert entity_key_some_movie is not None
        assert entity_key_some_movie != entity_key_matrix

        response = alice('о чем он')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) == 'movie_specific'
        assert get_disscussion_info_entity_key(response) == entity_key_some_movie

        response = alice('давай обсудим мультфильм')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_original_intent(response) == intent.LetsDiscussSomeMovie
        entity_key_some_mult = get_disscussion_info_entity_key(response)
        assert entity_key_some_mult is not None
        assert entity_key_some_mult != entity_key_matrix
        assert entity_key_some_mult != entity_key_some_movie
        question_suggest = response.suggests[1]['title']
        assert question_suggest.endswith('?')

        response = alice(question_suggest)
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_source(response) != 'proactivity'
        assert response.scenario_analytics_info.object('gc_response_info')['discussion_info']['entity_key'] == entity_key_some_mult


@pytest.mark.parametrize('surface', surface.actual_surfaces)
class TestMockingReply(object):

    owners = ('alzaharov',)

    mocked_answer = 'ответ ответ'

    @pytest.mark.experiments('hw_gc_mocked_reply={}'.format(base64.b64encode(mocked_answer.encode('utf-8')).decode('utf-8')))
    def test_mocker(self, alice):
        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.text == self.mocked_answer.capitalize()

        response = alice('давай поболтаем')
        assert response.scenario == scenario.GeneralConversation
        assert response.intent == intent.ExternalSkillGc
        assert get_gc_intent(response) == intent.PureGeneralConversation
        assert response.text != self.mocked_answer.capitalize()

        response = alice(DEFAULT_GC_TRIGGER_TEXT)
        assert response.text == self.mocked_answer.capitalize()
