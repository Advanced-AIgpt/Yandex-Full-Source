import re

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
class TestMicrointents(object):

    owners = ('flimsywhimsy', )
    regex_url_prefix = r'https://static-alice\.s3\.yandex\.net/led-production/emotions/v2'
    test_data = [
        ('привет', intent.Hello, True, 'joy'),
        ('доброе утро', intent.GoodMorning, True, 'joy'),
        ('расскажи шутку', intent.TellMeAJoke, True, '(joy|neutral)'),
        ('спокойной ночи', intent.GoodNight, False, '(love|joy|playful|wink)'),
        ('кто там', intent.WhoIsThere, True, 'surprise'),
    ]

    @pytest.mark.parametrize('surface', [surface.station_pro])
    @pytest.mark.parametrize('command, intent, should_listen, gif_name', test_data)
    def test_with_led(self, alice, command, intent, should_listen, gif_name):
        response = alice(command)
        assert response.intent == intent
        assert response.voice_response.should_listen == should_listen

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.ForceDisplayCardsDirective
        assert response.directives[1].name == directives.names.DrawLedScreenDirective

        seq = response.directives[1].payload.animation_sequence
        assert len(seq) == 2

        assert re.match(fr'{self.regex_url_prefix}/animate_in\.gif', seq[0].frontal_led_image)
        assert re.match(fr'{self.regex_url_prefix}/{gif_name}\.gif', seq[1].frontal_led_image)

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command, intent, should_listen', [
        test_case[:3] for test_case in test_data
    ])
    def test_without_led(self, alice, command, intent, should_listen):
        response = alice(command)
        assert response.intent == intent
        assert not response.directives
        assert response.voice_response.should_listen == should_listen
