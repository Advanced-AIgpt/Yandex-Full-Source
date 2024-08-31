import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice, server_action
from enum import Enum


logger = logging.getLogger(__name__)

EXPERIMENTS = [
    'hw_music_thin_client',
]

TEST_DEVICE_ID = 'device_id_1'


class DirectiveType(Enum):
    FIXED = 1
    ADJUSTABLE = 2


def make_env_state(directive_type):
    env_state = {
        'endpoints': [
            {
                'id': TEST_DEVICE_ID,
                'capabilities': [
                    {
                        'meta': {
                            'supported_directives': [],
                        },
                        'state': {
                            'preset_mode': 'MediaCorrection',
                        },
                        '@type': 'type.googleapis.com/NAlice.TEqualizerCapability',
                    },
                ],
            },
        ],
    }

    capability = env_state['endpoints'][0]['capabilities'][0]
    if directive_type == DirectiveType.FIXED:
        capability['meta']['supported_directives'].append('SetFixedEqualizerBandsDirectiveType')
        capability['parameters'] = {'fixed': {}}
    elif directive_type == DirectiveType.ADJUSTABLE:
        capability['meta']['supported_directives'].append('SetAdjustableEqualizerBandsDirectiveType')
        capability['parameters'] = {'adjustable': {}}

    return env_state

ENV_STATE_FIXED = make_env_state(DirectiveType.FIXED)
ENV_STATE_ADJUSTABLE = make_env_state(DirectiveType.ADJUSTABLE)

GET_EQUALIZER_SETTINGS_PAYLOAD = {
    'typed_semantic_frame': {
        'get_equalizer_settings_semantic_frame': {
        },
    },
    'analytics': {
        'origin': 'SmartSpeaker',
        'purpose': 'get_equalizer_settings',
    },
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.device_state(device_id=TEST_DEVICE_ID)
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class _TestsEqualizerBase:
    pass


@pytest.mark.parametrize('directive_name', [
    pytest.param('SetFixedEqualizerBandsDirective', marks=pytest.mark.environment_state(ENV_STATE_FIXED), id='fixed'),
    pytest.param('SetAdjustableEqualizerBandsDirective', marks=pytest.mark.environment_state(ENV_STATE_ADJUSTABLE), id='adjustable'),
])
class TestsEqualizer(_TestsEqualizerBase):

    def _assert_fixed_directive(self, directive, expected_gains=None):
        assert len(directive.Gains) == 5
        if expected_gains is not None:
            assert directive.Gains == expected_gains

    def _assert_adjustable_directive(self, directive, expected_gains=None):
        assert len(directive.Bands) == 5

        frequencies = [b.Frequency for b in directive.Bands]
        assert frequencies == [60, 230, 910, 3600, 14_000]

        widths = [b.Width for b in directive.Bands]
        assert widths == [90, 340, 1340, 5200, 13_000]

        if expected_gains is not None:
            gains = [b.Gain for b in directive.Bands]
            assert gains == expected_gains

    def _assert_directive(self, directive, directive_name, expected_gains=None):
        assert directive.HasField(directive_name)

        if directive_name == 'SetFixedEqualizerBandsDirective':
            self._assert_fixed_directive(getattr(directive, directive_name), expected_gains)
        elif directive_name == 'SetAdjustableEqualizerBandsDirective':
            self._assert_adjustable_directive(getattr(directive, directive_name), expected_gains)
        else:
            assert False, 'wrong directive name'

    def test_simple(self, alice, directive_name):
        r = alice(voice('включи du hast'))
        assert r.scenario_stages() == {'run', 'continue'}
        directives = r.continue_response.ResponseBody.Layout.Directives

        # order of directives is important!
        assert len(directives) == 2
        self._assert_directive(directives[0], directive_name)
        assert directives[1].HasField('AudioPlayDirective')

    def test_semantic_frame(self, alice, directive_name):
        # enable music
        r = alice(voice('включи музыку'))

        # try get equalizer directive
        r = alice(server_action(name='@@mm_semantic_frame', payload=GET_EQUALIZER_SETTINGS_PAYLOAD))
        assert r.scenario_stages() == {'run'}

        # check equalizer directive
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._assert_directive(directives[0], directive_name)

    def test_not_found_preset(self, alice, directive_name):
        # edmgenre
        r = alice(voice('включи WOAH WYR GEMI'))

        # try get equalizer directive
        r = alice(server_action(name='@@mm_semantic_frame', payload=GET_EQUALIZER_SETTINGS_PAYLOAD))
        assert r.scenario_stages() == {'run'}

        # check equalizer directive
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        self._assert_directive(directives[0], directive_name, expected_gains=[0, 0, 0, 0, 0])

    def test_irrelevant_semantic_frame(self, alice, directive_name):
        '''
        Музыка не проигрывается, эквалайзер не может ничего вернуть. Вернем пустой ответ
        без текста/голоса. Раньше ответ помечался irrelevant, но это не нужно.
        '''
        r = alice(server_action(name='@@mm_semantic_frame', payload=GET_EQUALIZER_SETTINGS_PAYLOAD))
        assert r.scenario_stages() == {'run'}
        assert not r.run_response.ResponseBody.HasField('Layout')
