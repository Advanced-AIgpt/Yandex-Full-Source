import re

import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
class TestPogodaLed(object):

    owners = ('sparkle', 'flimsywhimsy', 'abc:weatherbackendvteam', )
    regex_url_prefix = r'https://static-alice\.s3\.yandex\.net/led-production/weather/v\d+\.\d+/'

    @pytest.mark.parametrize('surface', [surface.station_pro])
    def test_with_led(self, alice):
        response = alice('погода')
        assert response.voice_response.should_listen

        assert len(response.directives) == 2
        assert response.directives[0].name == directives.names.ForceDisplayCardsDirective
        assert response.directives[1].name == directives.names.DrawLedScreenDirective

        seq = response.directives[1].payload.animation_sequence
        assert len(seq) == 2

        temp_part = seq[0]
        assert re.match(fr'{self.regex_url_prefix}temperature/-?\d+\.gif', temp_part.frontal_led_image)

        condition_part = seq[1]
        assert any(re.match(fr'{self.regex_url_prefix}{name}\.gif', condition_part.frontal_led_image) for name in [
            'sun', 'clouds',
            'rain_low', 'rain_hi', 'rain_medium',
            'snow_low', 'snow_hi', 'snow_medium',
            'snow_rain',
        ])

    @pytest.mark.parametrize('surface', [surface.station])
    def test_without_led(self, alice):
        response = alice('погода')
        assert not response.directives
        assert response.voice_response.should_listen
