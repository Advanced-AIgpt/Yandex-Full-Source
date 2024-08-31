import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [
    surface.automotive,
    surface.navi,
])
class TestSpeakerVoice(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1191
    https://testpalm.yandex-team.ru/testcase/alice-1602
    """

    owners = ('mihajlova', 'g:maps-auto-crossplatform')

    @pytest.mark.parametrize('command, sound_scheme', [
        ('включи мне голос Харламова', 'ru_kharlamov'),
        ('давай голос гарика', 'ru_kharlamov'),
        ('вруби голос басты', 'ru_basta'),
        ('хочу голос йоды', 'ru_starwars_light'),
        ('хочу озвучку уткина', 'ru_easter_egg'),
        ('включи диму', 'ru_male'),
        ('включи голос бузовой', 'ru_buzova'),
        ('включи свой голос', 'ru_alice'),
        ('включи голос прайма', 'ru_optimus'),
    ])
    def test_alice_1602(self, alice, command, sound_scheme):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.ChangeVoice
        assert response.text == 'Хорошо, сейчас включу.'
        assert response.directive.name == directives.names.OpenUriDirective
        assert response.directive.payload.uri == f'yandexnavi://set_sound_scheme?scheme={sound_scheme}&welcome=1'
        assert response.has_voice_response()
        assert response.output_speech_text == response.text
