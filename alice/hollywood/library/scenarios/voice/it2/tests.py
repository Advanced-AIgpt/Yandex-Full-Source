import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.memento.proto.api_pb2 import EConfigKey
from alice.memento.proto.user_configs_pb2 import TTtsWhisperConfig
from alice.protos.data.contextual_data_pb2 import TContextualData


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['voice']


def _get_tts_whisper_config(response_body):
    tts_whisper_config = TTtsWhisperConfig()

    memento_directive = response_body.ServerDirectives[0].MementoChangeUserObjectsDirective
    user_configs = memento_directive.UserObjects.UserConfigs

    assert len(user_configs) == 1
    assert user_configs[0].Key == EConfigKey.CK_TTS_WHISPER

    user_configs[0].Value.Unpack(tts_whisper_config)

    return tts_whisper_config


@pytest.mark.scenario(name='Voice', handle='voice')
class TestWhisperBase:
    pass


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
class TestWhisper(TestWhisperBase):

    @pytest.mark.memento({
        'UserConfigs': {
            'TtsWhisperConfig': {
                'Enabled': False,
            }
        }
    })
    def test_whisper_mode_on(self, alice):
        response = alice(voice('а давай шепотом поговорим'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'Сделала. Теперь если вы говорите шепотом я буду отвечать вам так же на умных колонках и в приложении Яндекса.'
        assert response_body.Layout.Cards[0].Text == 'Сделала. Теперь, если вы говорите шепотом, я буду отвечать вам так же на умных колонках и в приложении Яндекса.'
        assert response_body.AnalyticsInfo.Intent == 'whisper.turn_on'

        assert len(response_body.ServerDirectives) == 1
        assert _get_tts_whisper_config(response_body).Enabled

    @pytest.mark.memento({
        'UserConfigs': {
            'TtsWhisperConfig': {
                'Enabled': True,
            }
        }
    })
    def test_whisper_mode_off(self, alice):
        response = alice(voice('перестань говорить шепотом'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'Как скажете.'
        assert response_body.AnalyticsInfo.Intent == 'whisper.turn_off'

        assert len(response_body.ServerDirectives) == 1
        assert not _get_tts_whisper_config(response_body).Enabled

    def test_whisper_something(self, alice):
        response = alice(voice('а скажи что-нибудь шепотом'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech.startswith('<speaker is_whisper="true">')
        assert not response_body.Layout.Cards[0].Text.startswith('<speaker is_whisper="true">')
        assert response_body.AnalyticsInfo.Intent == 'whisper.say_something'

        assert not response_body.ServerDirectives

    def test_what_is_whisper(self, alice):
        response = alice(voice('что такое режим шепота'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом я отвечаю так же, на умных колонках и в приложении Яндекса. Включить?'
        assert response_body.Layout.Cards[0].Text == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом, я отвечаю так же на умных колонках и в приложении Яндекса. Включить?'
        assert response_body.Layout.ShouldListen
        assert response_body.AnalyticsInfo.Intent == 'whisper.what_is_it'

        assert not response_body.ServerDirectives

        assert len(response_body.FrameActions) == 2
        assert response_body.FrameActions['whisper_turn_on_action'].NluHint.FrameName == 'alice.proactivity.confirm'
        assert response_body.FrameActions['whisper_turn_on_action'].ParsedUtterance.TypedSemanticFrame.HasField('WhisperTurnOnSemanticFrame')
        assert response_body.FrameActions['decline_action'].NluHint.FrameName == 'alice.proactivity.decline'
        assert response_body.FrameActions['decline_action'].ParsedUtterance.TypedSemanticFrame.HasField('DoNothingSemanticFrame')

        response = alice(voice('включи'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'Сделала. Теперь если вы говорите шепотом я буду отвечать вам так же на умных колонках и в приложении Яндекса.'
        assert response_body.Layout.Cards[0].Text == 'Сделала. Теперь, если вы говорите шепотом, я буду отвечать вам так же на умных колонках и в приложении Яндекса.'
        assert response_body.AnalyticsInfo.Intent == 'whisper.turn_on'

        assert len(response_body.ServerDirectives) == 1
        assert _get_tts_whisper_config(response_body).Enabled

    @pytest.mark.memento({
        'UserConfigs': {
            'TtsWhisperConfig': {
                'Enabled': True,
            }
        }
    })
    def test_what_is_whisper_no_proactivity(self, alice):
        response = alice(voice('что такое режим шепота'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом я отвечаю так же, на умных колонках и в приложении Яндекса.'
        assert response_body.Layout.Cards[0].Text == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом, я отвечаю так же на умных колонках и в приложении Яндекса.'
        assert not response_body.Layout.ShouldListen
        assert response_body.AnalyticsInfo.Intent == 'whisper.what_is_it'

        assert not response_body.ServerDirectives
        assert not response_body.FrameActions


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
class TestWhisperModifiers(TestWhisperBase):
    def test_whisper_mode_on_modifier(self, alice):
        response = alice(voice('а давай шепотом поговорим', whisper=True))
        assert response.run_response.ResponseBody.ContextualData.Whisper.Hint == TContextualData.TWhisper.EHint.ForcedEnable

        response = alice(voice('а давай шепотом поговорим'))
        assert response.run_response.ResponseBody.ContextualData == TContextualData()

    def test_whisper_mode_off_modifier(self, alice):
        response = alice(voice('перестань говорить шепотом', whisper=True))
        assert response.run_response.ResponseBody.ContextualData.Whisper.Hint == TContextualData.TWhisper.EHint.ForcedDisable

        response = alice(voice('перестань говорить шепотом'))
        assert response.run_response.ResponseBody.ContextualData.Whisper.Hint == TContextualData.TWhisper.EHint.ForcedDisable


@pytest.mark.parametrize('surface', [surface.station, surface.searchapp])
class TestWhisperNoLogin(TestWhisperBase):
    def test_whisper_mode_on(self, alice):
        response = alice(voice('а давай шепотом поговорим'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'Для настройки режима шепота войдите в Яндекс.'
        assert response_body.AnalyticsInfo.Intent == 'whisper.turn_on'

        assert not response_body.ServerDirectives

    def test_whisper_mode_off(self, alice):
        response = alice(voice('перестань говорить шепотом'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'Для настройки режима шепота войдите в Яндекс.'
        assert response_body.AnalyticsInfo.Intent == 'whisper.turn_off'

        assert not response_body.ServerDirectives

    def test_whisper_something(self, alice):
        response = alice(voice('а скажи что-нибудь шепотом'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech.startswith('<speaker is_whisper="true">')
        assert not response_body.Layout.Cards[0].Text.startswith('<speaker is_whisper="true">')
        assert response_body.AnalyticsInfo.Intent == 'whisper.say_something'

        assert not response_body.ServerDirectives

    def test_what_is_whisper(self, alice):
        response = alice(voice('что такое режим шепота'))
        assert response.scenario_stages() == {'run'}

        response_body = response.run_response.ResponseBody
        assert response_body.Layout.OutputSpeech == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом я отвечаю так же, на умных колонках и в приложении Яндекса.'
        assert response_body.Layout.Cards[0].Text == 'У меня есть режим шепота. Это тихий режим, если вы говорите шепотом, я отвечаю так же на умных колонках и в приложении Яндекса.'
        assert not response_body.Layout.ShouldListen
        assert response_body.AnalyticsInfo.Intent == 'whisper.what_is_it'

        assert not response_body.ServerDirectives
        assert not response_body.FrameActions


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestWhisperLoginPrompt(TestWhisperBase):
    @pytest.mark.parametrize('command', [
        pytest.param('а давай шепотом поговорим', id='turn_on'),
        pytest.param('перестань говорить шепотом', id='turn_off'),
    ])
    def test_login_prompt(self, alice, command):
        response = alice(voice(command))
        directives = response.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].OpenUriDirective.Uri == 'yandex-auth://?theme=light'
