import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice, Scenario
from alice.hollywood.library.scenarios.music.it2.thin_client_helpers import (
    assert_audio_play_directive,
    get_first_track_id,
)
from alice.hollywood.library.scenarios.music.proto.music_memento_scenario_data_pb2 import TMusicScenarioMementoData
from alice.megamind.protos.scenarios.analytics_info_pb2 import TAnalyticsInfo
from conftest import get_scenario_state


TMusicEvent = TAnalyticsInfo.TEvent.TMusicEvent

DEFAULT_ANSWER = 'Включаю сказки.'
CHILD_AGE_PROMO_ANSWERS = [
    'Включаю сказки. Хотите, я подберу их под возраст ребёнка? Тогда скажите мне в приложении Яндекса в вашем телефоне: "Алиса, настрой сказки".',
    'Включаю сказки. Чтобы я подобрала её по возрасту, скажите мне в приложении Яндекса в вашем телефоне: "Алиса, настрой сказки".',
    'Включаю сказки. Кстати, её можно подобрать по возрасту ребёнка. Просто скажите мне в приложении Яндекса в вашем телефоне: "Алиса, настрой сказки".',
]
BEDTIMES_TIMER_DUCATION_SEC = 900


def _memento_child_age(
    age,
    epoch_seconds=1634582896,  # could be any other nonzero timestamp
):
    return {
        'UserConfigs': {
            'ChildAge': {
                'Age': age,
                'EpochSeconds': epoch_seconds,
            }
        }
    }


class AssertPlayerDirectiveMixin:
    def assert_player_directive(self, response):
        raise NotImplementedError()


class AssertThinPlayerDirectiveMixin(AssertPlayerDirectiveMixin):

    def assert_player_directive(self, response):
        assert_audio_play_directive(response.continue_response.ResponseBody.Layout.Directives)


class AssertDefaultPlayerDirectiveMixin(AssertPlayerDirectiveMixin):

    def assert_player_directive(self, response):
        layout = response.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.oauth(auth.YandexPlus)
class BaseFairyTalePlaylistTest(AssertPlayerDirectiveMixin):

    def _get_playlist(self, analytics_info):
        for event in analytics_info.Events:
            if event.MusicEvent.AnswerType == TMusicEvent.EAnswerType.Playlist:
                return event
        return None

    def _get_scenario_data(self, any):
        scenario_data = TMusicScenarioMementoData()
        any.Unpack(scenario_data)
        return scenario_data

    @pytest.mark.parametrize('age, expected_playlist_id', [
        pytest.param(None, '970829816:1039', id='default_playlist'),
        pytest.param(3, '970829816:1002', id='babies', marks=pytest.mark.memento(_memento_child_age(3))),
        pytest.param(4, '103372440:1872', id='kids_lower_bound', marks=pytest.mark.memento(_memento_child_age(4))),
        pytest.param(7, '103372440:1872', id='kids_upper_bound', marks=pytest.mark.memento(_memento_child_age(7))),
        pytest.param(8, '970829816:1045', id='children', marks=pytest.mark.memento(_memento_child_age(8))),
        pytest.param(8, '970829816:1039', id='default_proto_default_playlist', marks=pytest.mark.memento(_memento_child_age(0, epoch_seconds=0))),
    ])
    def test_playlist(self, alice, age, expected_playlist_id):
        response = alice(voice('расскажи сказку'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == DEFAULT_ANSWER or (age is None and layout.OutputSpeech in CHILD_AGE_PROMO_ANSWERS)
        assert response.scenario_stages() == {'run', 'continue'}

        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_fairy_tale"
        assert analytics.ProductScenarioName == "music_fairy_tale"

        self.assert_player_directive(response)
        playlist = self._get_playlist(analytics)
        assert playlist is not None
        assert playlist.MusicEvent.Id == expected_playlist_id

    @pytest.mark.experiments(
        'fairy_tales_age_selector_force_always_send_push',
    )
    def test_child_age_promo_push_message(self, alice):
        response = alice(voice('расскажи сказку'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech in CHILD_AGE_PROMO_ANSWERS
        assert response.scenario_stages() == {'run', 'continue'}
        self.assert_player_directive(response)

    @pytest.mark.experiments('fairy_tales_disable_shuffle')
    def test_disable_shuffle(self, alice):
        response = alice(voice('расскажи сказку'))
        first_track_id = get_first_track_id(response.continue_response.ResponseBody.AnalyticsInfo)
        second_response = alice(voice('расскажи сказку'))
        assert first_track_id == get_first_track_id(second_response.continue_response.ResponseBody.AnalyticsInfo)

    def test_shuffle(self, alice):
        response = alice(voice('расскажи сказку'))
        first_track_id = get_first_track_id(response.continue_response.ResponseBody.AnalyticsInfo)
        second_response = alice(voice('расскажи сказку'))
        assert first_track_id != get_first_track_id(second_response.continue_response.ResponseBody.AnalyticsInfo)

    @pytest.mark.parametrize('query', [
        pytest.param('перемешай', id='shuffle'),
        pytest.param('повторяй этот трек', id='repeat'),
        pytest.param('включи трек сначала', id='replay')
    ])
    def test_player_commands(self, alice, query):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice(query))
        if response.scenario_stages() == {'run'}:
            resp = response.run_response
        elif response.scenario_stages() == {'run', 'continue'}:
            resp = response.continue_response
        analytics = resp.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(resp)
        assert state.ProductScenarioName == "music_fairy_tale"

    def test_continue(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        r = alice(voice('поставь на паузу', scenario=Scenario('Commands', 'fast_command')))
        assert r.scenario_stages() == {'run'}

        response = alice(voice('продолжи'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

    def test_clear_scenario_name_after_fairytale(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('перемешай'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('включи спокойную муыку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName != "music_fairy_tale"

        response = alice(voice('перемешай'))
        # в "тонкой" музыке повторный "перемешай" ответит "да тут и так все вперемешку" в run-стадии
        if response.scenario_stages() == {'run'}:
            analytics = response.run_response.ResponseBody.AnalyticsInfo
        elif response.scenario_stages() == {'run', 'continue'}:
            analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName != "music_fairy_tale"


@pytest.mark.experiments(
    'hw_music_thin_client_fairy_tale_playlists',
    'hw_music_thin_client',
)
class TestThinMusicFairyTalePlaylists(BaseFairyTalePlaylistTest, AssertThinPlayerDirectiveMixin):

    def test_what_is_playing(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('что сейчас играет'))
        assert response.scenario_stages() == {'run'}
        layout = response.run_response.ResponseBody.Layout
        assert layout.OutputSpeech.startswith("<[domain music]> Сейчас играет сказка")
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.run_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        return str(response)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_like(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice("поставь лайк"))
        assert response.scenario_stages() == {'run', 'commit'}
        state = get_scenario_state(response.run_response.CommitCandidate)
        assert state.ProductScenarioName == "music_fairy_tale"

    @pytest.mark.experiments(
        'fairy_tales_bedtime_tales'
    )
    def test_bedtime_tales(self, alice):
        response = alice(voice('расскажи сказку на ночь'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == "Включаю сказки н+аночь. Колонка выключится, когда сказка закончится, но не раньше, чем через 15 минут."
        assert response.scenario_stages() == {'run', 'continue'}

        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_fairy_tale"
        assert analytics.ProductScenarioName == "music_fairy_tale"

        self.assert_player_directive(response)
        playlist = self._get_playlist(analytics)
        assert playlist is not None
        assert playlist.MusicEvent.Id == '970829816:1007'

        state = get_scenario_state(response.continue_response)
        assert state.HasField('FairytaleTurnOffTimer')
        turnOffTimer = state.FairytaleTurnOffTimer
        assert turnOffTimer.Duration == BEDTIMES_TIMER_DUCATION_SEC * 1000

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_linear_albums(self, alice):
        response = alice(voice('включи волшебник изумрудного города часть 2'))
        assert response.scenario_stages() == {'run', 'continue'}

        directives = response.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Волшебник Изумрудного города. Часть 2', sub_title='Аудиокнига в кармане')

        response = alice(voice('следующий трек'))
        assert response.scenario_stages() == {'run', 'continue'}
        directives = response.continue_response.ResponseBody.Layout.Directives
        assert_audio_play_directive(directives, title='Волшебник Изумрудного города. Часть 3', sub_title='Аудиокнига в кармане')


class TestDefaultMusicPlaylists(BaseFairyTalePlaylistTest, AssertDefaultPlayerDirectiveMixin):

    def test_what_is_playing(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('что сейчас играет'))
        assert response.scenario_stages() == {'run', 'continue'}
        # TODO(sparkle): check it out
        # analytics = response.continue_response.ResponseBody.AnalyticsInfo
        # assert analytics.ProductScenarioName == "music_fairy_tale"
        # state = get_scenario_state(response.continue_response)
        # assert state.ProductScenarioName == "music_fairy_tale"

        return str(response)  # XXX(sparkle): added for HOLLYWOOD-935

    def test_next_track(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('включи следующую'))
        assert response.scenario_stages() == {'run'}
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.run_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice('включи предыдущую'))
        assert response.scenario_stages() == {'run'}
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.run_response)
        assert state.ProductScenarioName == "music_fairy_tale"

    def test_like(self, alice):
        response = alice(voice('расскажи сказку'))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

        response = alice(voice("поставь лайк"))
        assert response.scenario_stages() == {'run', 'continue'}
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

    @pytest.mark.experiments(
        'fairy_tales_bedtime_tales'
    )
    def test_bedtime_tales(self, alice):
        response = alice(voice('расскажи сказку на ночь'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == "Включаю сказки н+аночь. Колонка выключится через 15 минут."
        assert response.scenario_stages() == {'run', 'continue'}

        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_fairy_tale"
        assert analytics.ProductScenarioName == "music_fairy_tale"

        playlist = self._get_playlist(analytics)
        assert playlist is not None
        assert playlist.MusicEvent.Id == '970829816:1007'

        directives = layout.Directives
        assert len(directives) == 3
        assert layout.Directives[0].HasField('MusicPlayDirective')
        assert layout.Directives[1].HasField('SetTimerDirective')
        assert layout.Directives[2].HasField('TtsPlayPlaceholderDirective')
        timer = directives[1].SetTimerDirective
        assert timer.Duration == BEDTIMES_TIMER_DUCATION_SEC
        assert len(timer.Directives) == 4
        assert timer.Directives[0].HasField('PlayerPauseDirective')
        assert timer.Directives[1].HasField('ClearQueueDirective')
        assert timer.Directives[2].HasField('GoHomeDirective')
        assert timer.Directives[3].HasField('ScreenOffDirective')

        serverDirectives = response.continue_response.ResponseBody.ServerDirectives
        assert serverDirectives[0].HasField('MementoChangeUserObjectsDirective')
        memento = serverDirectives[0].MementoChangeUserObjectsDirective
        scenario_data = self._get_scenario_data(memento.UserObjects.ScenarioData)
        assert scenario_data.FairyTalesData
        assert scenario_data.FairyTalesData.BedtimeTalesOnboardingCounter == 1

    def test_linear_albums(self, alice):
        response = alice(voice('включи волшебник изумрудного города часть 2'))
        assert response.scenario_stages() == {'run', 'continue'}

        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Events[3].HasField('MusicEvent')
        musicEvent = analytics.Events[3].MusicEvent
        assert musicEvent.AnswerType == 2  # Album
        assert musicEvent.Id == "11551379"

        return str(response)


@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.oauth(auth.YandexPlus)
class TestFairyTalesOnSearchApp:

    @pytest.mark.xfail(reason='Test is dead after recanonization')
    def test_linear_albums(self, alice):
        response = alice(voice('включи волшебник изумрудного города часть 2'))
        assert response.scenario_stages() == {'run', 'continue'}

        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Events[3].HasField('MusicEvent')
        musicEvent = analytics.Events[3].MusicEvent
        assert musicEvent.AnswerType == 2  # Album
        assert musicEvent.Id == "11551379"

        return str(response)


@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'hw_music_fairy_tales_enable_ondemand',
    'bg_fairy_tale_text_slot',
    'hw_log_nlg',
)
class BaseTestOndemandFairyTales(AssertPlayerDirectiveMixin):

    @pytest.mark.parametrize('query,fairy_tale', [
        pytest.param('включи сказку колобок', 'колобок', id='kolobok'),
        pytest.param('включи сказку Алёша Попович и Тугарин Змеевич', 'Алёша Попович и Тугарин Змеевич', id='genre_folk'),
        pytest.param('включи сказку мумий тролль и комета два', 'муми-тролль', id='multiple_fairy_tale_slots'),
        pytest.param('поставь сказку про хитрую лису', 'лис', id='tricky_fox'),
    ])
    def test_ondemand(self, alice, query, fairy_tale):
        response = alice(voice(query))
        assert response.scenario_stages() == {'run', 'continue'}
        layout = response.continue_response.ResponseBody.Layout
        output_speech = layout.OutputSpeech.lower()
        assert output_speech.count('сказк') == 1, f'Output speech doesn\'t contain "fairy tale" word or contains it more than once: "{output_speech}"'
        assert fairy_tale.lower() in output_speech
        self.assert_player_directive(response)
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_fairy_tale"
        assert analytics.ProductScenarioName == "music_fairy_tale"
        state = get_scenario_state(response.continue_response)
        assert state.ProductScenarioName == "music_fairy_tale"

    def test_unknown_fairy_tale(self, alice):
        response = alice(voice('включи сказку от группы айспик'))
        layout = response.run_response.ResponseBody.Layout
        output_speech = layout.OutputSpeech.lower()
        assert re.match('.*сказки.*нет.*', output_speech) is not None, f'Unexpected unknown fairy tale response: "{output_speech}"'
        analytics = response.run_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_fairy_tale"
        assert analytics.ProductScenarioName == "music_fairy_tale"

    def test_child_song(self, alice):
        response = alice(voice('включи песенку малышарики красное'))
        layout = response.continue_response.ResponseBody.Layout
        output_speech = layout.OutputSpeech.lower()
        assert 'сказк' not in output_speech, f'Unexpected fairy tale text: {layout.OutputSpeech}'
        analytics = response.continue_response.ResponseBody.AnalyticsInfo
        assert analytics.Intent == "personal_assistant.scenarios.music_play"
        assert analytics.ProductScenarioName == "music"


@pytest.mark.experiments('hw_music_thin_client_fairy_tale_ondemand', 'hw_music_thin_client')
class TestThinOndemandFairyTales(BaseTestOndemandFairyTales, AssertThinPlayerDirectiveMixin):
    pass


class TestDefaultOndemandFairyTales(BaseTestOndemandFairyTales, AssertDefaultPlayerDirectiveMixin):
    pass


@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.oauth(auth.YandexPlus)
class TestFairyTalesOnTv:
    @pytest.mark.experiments(
        'fairy_tales_age_selector_force_always_send_push',
    )
    def test_child_age_promo_push_message_disabled(self, alice):
        response = alice(voice('расскажи сказку'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю'
        assert response.scenario_stages() == {'run', 'continue'}

    @pytest.mark.supported_features(audio_client=None)  # This emulates some old smart_tvs witout audio_client
    @pytest.mark.experiments(
        'hw_music_thin_client_fairy_tale_playlists',
        'hw_music_thin_client',
    )
    def test_thin_player_flags_are_ignored(self, alice):
        response = alice(voice('расскажи сказку'))
        layout = response.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю'
        assert response.scenario_stages() == {'run', 'continue'}
        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('MusicPlayDirective')
