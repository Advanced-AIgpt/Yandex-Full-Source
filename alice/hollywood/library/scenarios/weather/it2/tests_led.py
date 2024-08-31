import re

import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Weather', handle='weather')
class TestsLed:

    regex_url_prefix = r'https://static-alice\.s3\.yandex\.net/led-production/weather/v\d+\.\d+/'

    @pytest.mark.parametrize('surface', [surface.station_pro])
    def test_with_led(self, alice):
        r = alice(voice('погода'))
        assert r.run_response.ResponseBody.Layout.ShouldListen

        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert directives[0].HasField('DrawLedScreenDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')

        seq = directives[0].DrawLedScreenDirective.DrawItem
        assert len(seq) == 2

        temp_part = seq[0]
        assert re.match(fr'{self.regex_url_prefix}temperature/-?\d+\.gif', temp_part.FrontalLedImage)

        condition_part = seq[1]
        assert any(re.match(fr'{self.regex_url_prefix}{name}\.gif', condition_part.FrontalLedImage) for name in [
            'sun', 'clouds',
            'rain_low', 'rain_hi', 'rain_medium',
            'snow_low', 'snow_hi', 'snow_medium',
            'snow_rain',
        ])

    @pytest.mark.parametrize('surface', [surface.station])
    def test_without_led(self, alice):
        r = alice(voice('погода'))
        assert not r.run_response.ResponseBody.Layout.Directives
        assert r.run_response.ResponseBody.Layout.ShouldListen


@pytest.mark.scenario(name='Weather', handle='weather')
@pytest.mark.supported_features('scled_display')
@pytest.mark.parametrize('surface', [surface.loudspeaker])
class TestScled:

    def test_smoke(self, alice):
        r = alice(voice('погода'))
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 2
        assert directives[0].HasField('DrawScledAnimationsDirective')
        assert directives[1].HasField('TtsPlayPlaceholderDirective')
        return str(directives)
