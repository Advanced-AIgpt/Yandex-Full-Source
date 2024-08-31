import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.protos.endpoint.capability_pb2 import TAnimationCapability


TEST_DEVICE_ID = 'device_id_1'

ENV_STATE = {
    'endpoints': [
        {
            'id': TEST_DEVICE_ID,
            'capabilities': [
                {
                    'parameters': {
                        'supported_formats': [
                            'S3Url',
                        ],
                    },
                    '@type': 'type.googleapis.com/NAlice.TAnimationCapability',
                },
            ],
        },
    ],
}


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.environment_state(ENV_STATE)
@pytest.mark.device_state(device_id=TEST_DEVICE_ID)
@pytest.mark.parametrize('surface', [surface.station])
class TestsAnimations:

    @staticmethod
    def _find_directive_with_index(directives, directive_name):
        for i, d in enumerate(directives):
            if d.HasField(directive_name):
                return i, getattr(d, directive_name)
        return None, None

    def test_smoke(self, alice):
        r = alice(voice('погода в питере'))

        directives = r.run_response.ResponseBody.Layout.Directives

        # Проверка директивы анимации на корректность
        draw_animation_index, draw_animation_directive = \
                self._find_directive_with_index(directives, 'DrawAnimationDirective')
        assert draw_animation_directive

        assert len(draw_animation_directive.Animations) == 1
        assert draw_animation_directive.Animations[0].S3Directory.Bucket == 'https://quasar.s3.yandex.net'
        assert draw_animation_directive.SpeakingAnimationPolicy == TAnimationCapability.TDrawAnimationDirective.ESpeakingAnimationPolicy.PlaySpeakingEndOfTts

        # Проверка, что tts_play_placeholder находится ПОСЛЕ директивы анимации
        tts_play_placeholder_index, tts_play_placeholder_directive = \
                self._find_directive_with_index(directives, 'TtsPlayPlaceholderDirective')

        assert tts_play_placeholder_directive
        assert draw_animation_index < tts_play_placeholder_index
