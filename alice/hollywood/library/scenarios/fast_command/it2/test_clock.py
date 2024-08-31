import pytest
from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['fast_command']


UNSUPPORTED_TURN_OFF_OPERATION_REPLIES = [
    'Счастливые часов не наблюдают, а я пока не умею их выключать на какой-то период. Простите!',
    'Если бы я умела так, то выключила бы, но я так пока не умею.',
    'Простите, я пока не умею выключать часы на период, могу только до момента, пока вы не попросите включить.',
]

UNSUPPORTED_TURN_ON_OPERATION_REPLIES = [
    'Если бы я умела так, то включила бы, но я так пока не умею.',
    'Простите, я пока не умею включать часы на период, могу только до момента, пока вы не попросите выключить.',
]


class Intents:
    TurnOn = 'alice.clock_face_control.turn_on'
    TurnOff = 'alice.clock_face_control.turn_off'
    DelayedTurnOnCant = 'alice.clock_face_control.delayed_turn_on_cant'
    DelayedTurnOffCant = 'alice.clock_face_control.delayed_turn_off_cant'


@pytest.mark.scenario(name='Commands', handle='fast_command')
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
@pytest.mark.supported_features('clock_display')
@pytest.mark.experiments(
    'clock_face_control_turn_on',
    'clock_face_control_turn_off',
    'bg_exp_enable_clock_face_turn_on_granet',
    'bg_exp_enable_clock_face_turn_off_granet'
)
class _TestClockBase:
    pass


class TestClock(_TestClockBase):

    CLOCK_ENABLED_DEVICE_STATE = {
        'clock_display_state': {
            'clock_enabled': True,
        },
    }

    # this is the default device state
    CLOCK_DISABLED_DEVICE_STATE = {
        'clock_display_state': {
            'clock_enabled': False,
        },
    }

    @pytest.mark.device_state(CLOCK_ENABLED_DEVICE_STATE)
    @pytest.mark.parametrize('command, directive_name, intent', [
        pytest.param(
            'включи часы',
            'ShowClockDirective',
            Intents.TurnOn,
            id='turn_on',
        ),
        pytest.param(
            'выключи часы',
            'HideClockDirective',
            Intents.TurnOff,
            id='turn_off',
        ),
    ])
    def test_base_clock_face_command(self, alice, command, directive_name, intent):
        r = alice(voice(command))
        body = r.run_response.ResponseBody
        assert len(body.Layout.Directives) == 1
        assert body.Layout.Directives[0].HasField(directive_name)
        assert not body.Layout.OutputSpeech
        assert body.AnalyticsInfo.ProductScenarioName == 'led_clock_commands'
        assert body.AnalyticsInfo.Intent == intent

    @pytest.mark.device_state(CLOCK_DISABLED_DEVICE_STATE)
    def test_already_turned_off(self, alice):
        r = alice(voice('выключи часы'))
        layout = r.run_response.ResponseBody.Layout
        assert not layout.Directives
        assert layout.OutputSpeech == 'Уже выключены.'

    @pytest.mark.experiments('clock_face_control_unsupported_operation_nlg_response')
    @pytest.mark.parametrize('command, replies, intent', [
        pytest.param(
            'включи часы утром',
            UNSUPPORTED_TURN_ON_OPERATION_REPLIES,
            Intents.DelayedTurnOnCant,
            id='turn_on',
        ),
        pytest.param(
            'выключи часы до утра',
            UNSUPPORTED_TURN_OFF_OPERATION_REPLIES,
            Intents.DelayedTurnOffCant,
            id='turn_off',
        ),
    ])
    def test_delayed_clock_face_command(self, alice, command, replies, intent):
        r = alice(voice(command))
        body = r.run_response.ResponseBody
        assert not body.Layout.Directives
        assert body.Layout.OutputSpeech in replies
        assert body.AnalyticsInfo.ProductScenarioName == 'led_clock_commands'
        assert body.AnalyticsInfo.Intent == intent

    @pytest.mark.supported_features('scled_display')
    def test_scled_animation(self, alice):
        r = alice(voice('включи часы'))
        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech

        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('DrawScledAnimationsDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('ShowClockDirective')

        return str(layout.Directives[0].DrawScledAnimationsDirective)

    @pytest.mark.supported_features('led_display')
    def test_led_animation(self, alice):
        r = alice(voice('включи часы'))
        layout = r.run_response.ResponseBody.Layout
        assert not layout.OutputSpeech

        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('DrawLedScreenDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('ShowClockDirective')

        return str(layout.Directives[0].DrawLedScreenDirective)
