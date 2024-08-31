import pytest

from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint
from alice.memento.proto.api_pb2 import EConfigKey
from alice.protos.data.proactivity.tag_stats_pb2 import TTagStatsStorage
from dj.services.alisa_skills.server.proto.client.onboarding_response_pb2 import TOnboardingResponse
from dj.services.alisa_skills.server.proto.client.proactivity_request_pb2 import TProactivityRequest
from google.protobuf.json_format import MessageToDict


what_can_you_do_stubber = create_stubber_fixture(
    'skills-rec-test.alice.yandex.net', 80,
    [StubberEndpoint('/what_can_you_do', ['POST'])],
    type_to_proto={
        'pseudo_grpc_request': TProactivityRequest,
        'pseudo_grpc_response': TOnboardingResponse,
    },
    stubs_subdir='what_can_you_do',
    header_filter_regexps=['x-request-id', 'content-length']
)


@pytest.fixture(scope="function")
def srcrwr_params(what_can_you_do_stubber):
    return {
        'SKILL_PROACTIVITY_HTTP': f'localhost:{what_can_you_do_stubber.port}',
    }


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['onboarding']


DEVICE_STATE_MUSIC_PLAYING = {
    'is_tv_plugged_in': False,
    'music': {
        'player': {
            'pause': False,
        },
        'currently_playing': {
            'track_id': 'track_id_1'
        }
    }
}

DEVICE_STATE_TV_STREAM = {
    'is_tv_plugged_in': True,
    'video': {
        'current_screen': 'video_player',
        'currently_playing': {
            'item': {
                'type': 'tv_stream',
                'name': 'tv-stream-item-name',
            }
        }
    }
}

DEVICE_STATE_RADIO_PLAYING = {
    'is_tv_plugged_in': False,
    "radio": {
        "player": {
            "pause": False
        },
        "currently_playing": {
            "radioTitle": "Культура",
            "radioId": "kultura"
        },
        "playlist_owner": ""
    }
}

SKILLS_REQUEST_NODE_NAME = 'ALICE_SKILLS_PROXY'


def _assert_what_can_you_do_response(response_body, phrase, next_phrase_index,
                                     actions=['action_what_can_you_do_next', 'action_what_can_you_do_decline']):
    assert response_body.Layout.OutputSpeech.startswith(phrase)
    assert response_body.Layout.ShouldListen
    assert response_body.AnalyticsInfo.ProductScenarioName == 'onboarding'
    assert response_body.AnalyticsInfo.Intent == 'alice.onboarding.what_can_you_do'

    assert len(response_body.FrameActions) == len(actions)
    for action in actions:
        assert action in response_body.FrameActions

    if 'action_what_can_you_do_next' in response_body.FrameActions:
        action_next = response_body.FrameActions['action_what_can_you_do_next']
        assert action_next.NluHint.FrameName == 'alice.proactivity.tell_me_more'
        frame = action_next.ParsedUtterance.TypedSemanticFrame
        assert frame.HasField('OnboardingWhatCanYouDoSemanticFrame')
        assert frame.OnboardingWhatCanYouDoSemanticFrame.PhraseIndex.UInt32Value == next_phrase_index

    if 'action_what_can_you_do_decline' in response_body.FrameActions:
        action_decline = response_body.FrameActions['action_what_can_you_do_decline']
        assert action_decline.NluHint.FrameName == 'alice.proactivity.decline'
        frame_decline = action_decline.ParsedUtterance.TypedSemanticFrame
        assert frame_decline.HasField('DoNothingSemanticFrame')

    if 'action_what_can_you_do_stop' in response_body.FrameActions:
        action_decline = response_body.FrameActions['action_what_can_you_do_stop']
        assert action_decline.NluHint.FrameName == 'personal_assistant.scenarios.player.pause'
        frame_decline = action_decline.ParsedUtterance.TypedSemanticFrame
        assert frame_decline.HasField('DoNothingSemanticFrame')


def _try_find_server_directive(response_body, directive_type):
    for d in response_body.ServerDirectives:
        if d.HasField(directive_type):
            return getattr(d, directive_type)
    return None


def _find_server_directive(response_body, directive_type):
    d = _try_find_server_directive(response_body, directive_type)
    if d is None:
        raise AssertionError('{} not found in response'.format(directive_type))
    return d


def _get_user_config_from_memento_directive(r, key, type):
    memento = _find_server_directive(r.run_response.ResponseBody, 'MementoChangeUserObjectsDirective')
    for c in memento.UserObjects.UserConfigs:
        if c.Key == key:
            v = type()
            c.Value.Unpack(v)
            return v
    return None


def _get_memento(r):
    tag_stats = _get_user_config_from_memento_directive(r, EConfigKey.CK_PROACTIVITY_TAG_STATS, TTagStatsStorage)
    return {
        "UserConfigs": {
            "ProactivityTagStats": MessageToDict(tag_stats)
        }
    }


@pytest.mark.scenario(name='Onboarding', handle='onboarding')
class TestsBase:
    pass


class TestsWhatCanYouDoUnsupported(TestsBase):

    supported_surfaces = surface.smart_speakers

    def _assert_unsupported(self, run_response):
        assert run_response.Features.IsIrrelevant
        assert MessageToDict(run_response.ResponseBody.AnalyticsInfo) == {
            'product_scenario_name': 'onboarding',
            'nlg_render_history_records': [
                {'template_name': 'onboarding_common', 'phrase_name': 'notsupported', 'language': 'L_RUS'}
            ]
        }

    def _assert_nothing_to_do(self, run_response):
        assert run_response.Features.IsIrrelevant
        assert MessageToDict(run_response.ResponseBody.AnalyticsInfo) == {
            'product_scenario_name': 'onboarding',
            'nlg_render_history_records': [
                {'template_name': 'onboarding_common', 'phrase_name': 'nothing_to_do', 'language': 'L_RUS'}
            ]
        }

    @pytest.mark.parametrize('surface', list(set(surface.actual_surfaces) - set(supported_surfaces)))
    @pytest.mark.experiments('hw_onboarding_enable_what_can_you_do')
    def test_unsupported_surface(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        self._assert_unsupported(r.run_response)

    @pytest.mark.parametrize('surface', supported_surfaces)
    @pytest.mark.experiments('hw_onboarding_disable_what_can_you_do')
    def test_disabled_by_flag(self, alice):
        r = alice(voice('Что ты умеешь'))
        self._assert_nothing_to_do(r.run_response)


@pytest.mark.parametrize('surface', surface.smart_speakers)
class TestsWhatCanYouDoQuasar(TestsBase):

    @pytest.mark.device_state({
        'is_tv_plugged_in': True,
        'video': {'current_screen': 'main'}
    })
    def test_quasar_main_with_tv(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                     'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1)

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Дальше'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по РБК или программу ТНТ.',
                                              next_phrase_index=3)

        r = alice(voice('Продолжай'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите.',
                                              next_phrase_index=4)

    @pytest.mark.device_state({
        'is_tv_plugged_in': True,
        'video': {'current_screen': 'mordovia_webview'}
    })
    def test_quasar_mordovia_webview(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                     'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1)

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Дальше'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по РБК или программу ТНТ.',
                                              next_phrase_index=3)

        r = alice(voice('Продолжай'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите.',
                                              next_phrase_index=4)

    @pytest.mark.device_state(is_tv_plugged_in=False)
    def test_quasar_main_no_tv(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                     'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1)

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Дальше'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по Первому каналу или ТНТ.',
                                              next_phrase_index=3)

        r = alice(voice('Продолжай'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу подсказать вам лучший маршрут из точки А в точку Б.',
                                              next_phrase_index=4)

    @pytest.mark.device_state({
        'is_tv_plugged_in': True,
        'video': {'current_screen': 'gallery'}
    })
    def test_quasar_gallery(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра"',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Рассказывай'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по РБК или программу ТНТ.',
                                              next_phrase_index=3)

    @pytest.mark.device_state(DEVICE_STATE_MUSIC_PLAYING)
    def test_quasar_music_playing(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра"',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

    @pytest.mark.device_state(DEVICE_STATE_RADIO_PLAYING)
    def test_quasar_radio_playing(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра"',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

    @pytest.mark.device_state(DEVICE_STATE_TV_STREAM)
    def test_quasar_tv_stream(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра"',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

    @pytest.mark.device_state({
        'is_tv_plugged_in': True,
        'video': {'current_screen': 'bluetooth'}
    })
    def test_quasar_unknown_screen(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1)

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

    @pytest.mark.device_state(DEVICE_STATE_TV_STREAM)
    def test_quasar_main(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                     'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1)

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Дальше'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по РБК или программу ТНТ.',
                                              next_phrase_index=3)

    @pytest.mark.device_state(DEVICE_STATE_TV_STREAM)
    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_quasar_tv_stream_disable_main(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете нажать паузу и вернуться к просмотру позже. Но это прямой эфир, так что перемотать в нужное место не выйдет.',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете нажать паузу и вернуться к просмотру позже. Но это прямой эфир, так что перемотать в нужное место не выйдет.',
                                              next_phrase_index=2)

    @pytest.mark.device_state(DEVICE_STATE_RADIO_PLAYING)
    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_quasar_radio_playing_disable_main(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Чтобы переключаться между радиостанциями, говорите «следующая станция» или «предыдущая станция».',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Скажите название станции или назовите частоту. Радио можно ставить на паузу, чтобы включить позже. '
                                                'К сожалению, перемотать ничего не получится — это прямой эфир.',
                                              next_phrase_index=2)

    @pytest.mark.device_state(DEVICE_STATE_MUSIC_PLAYING)
    @pytest.mark.experiments('hw_what_can_you_do_switch_phrases')
    def test_quasar_music_playing_disable_main(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете перейти к следующей композиции, сказав «следующий трек» или остановить воспроизведение, сказав «пауза».',
                                              next_phrase_index=1)

        r = alice(voice('Да'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу поставить вам любую музыку, например просто скажите «поставь музыку для танцев».',
                                              next_phrase_index=2)

    @pytest.mark.device_state(is_tv_plugged_in=False)
    @pytest.mark.experiments('hw_what_can_you_do_dont_stop_on_decline')
    def test_quasar_stop_action(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Вы можете поставить таймер или будильник. '
                                                     'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                              next_phrase_index=1,
                                              actions=['action_what_can_you_do_next', 'action_what_can_you_do_decline', 'action_what_can_you_do_stop'])

        r = alice(voice('Еще'))
        assert r.scenario_stages() == {'run'}
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2,
                                              actions=['action_what_can_you_do_next', 'action_what_can_you_do_decline', 'action_what_can_you_do_stop'])


@pytest.mark.experiments('hw_what_can_you_do_use_skillrec')
@pytest.mark.parametrize('surface', [surface.station])
class TestsWhatCanYouDoQuasarSkillsRec(TestsBase):
    def test_quasar(self, alice):
        r = alice(voice('Что ты умеешь'))
        assert r.scenario_stages() == {'run'}
        assert r.sources_dump.get_http_request(SKILLS_REQUEST_NODE_NAME)
        assert r.sources_dump.get_http_response(SKILLS_REQUEST_NODE_NAME)
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                         phrase='Вы можете поставить таймер или будильник. '
                                                'Например, скажите: "Поставь таймер на 5 минут" или "Поставь будильник на 9 утра".',
                                         next_phrase_index=1)

        r = alice(voice('Еще', memento=_get_memento(r)))
        assert r.scenario_stages() == {'run'}
        assert r.sources_dump.get_http_request(SKILLS_REQUEST_NODE_NAME)
        assert r.sources_dump.get_http_response(SKILLS_REQUEST_NODE_NAME)
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу напомнить вам поздравить с днем рождения врага или покормить хомячка.',
                                              next_phrase_index=2)

        r = alice(voice('Что ты умеешь', memento=_get_memento(r)))
        assert r.scenario_stages() == {'run'}
        assert r.sources_dump.get_http_request(SKILLS_REQUEST_NODE_NAME)
        assert r.sources_dump.get_http_response(SKILLS_REQUEST_NODE_NAME)
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я знаю телепрограмму, вы всегда можете спросить меня, что идет по РБК или программу ТНТ.',
                                              next_phrase_index=1)

        r = alice(voice('Продолжай', memento=_get_memento(r)))
        assert r.scenario_stages() == {'run'}
        assert r.sources_dump.get_http_request(SKILLS_REQUEST_NODE_NAME)
        assert r.sources_dump.get_http_response(SKILLS_REQUEST_NODE_NAME)
        _assert_what_can_you_do_response(r.run_response.ResponseBody,
                                              phrase='Я могу включить вам русскую драму или американскую трагедию - словом, что попр+осите.',
                                              next_phrase_index=2)
