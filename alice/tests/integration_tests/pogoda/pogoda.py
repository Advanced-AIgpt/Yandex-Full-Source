import pytest

import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
from .common import PogodaDialogBase


@pytest.mark.region(region.Moscow)
@pytest.mark.parametrize('surface', [surface.station])
class TestPogoda(PogodaDialogBase):

    owners = ('abc:weatherbackendvteam', )

    def test_multistep_unknown_where(self, alice):
        response = alice('погода')
        assert response.intent.startswith(intent.GetWeather)
        assert 'Сейчас в Москве' in response.text

        response = alice('а во владивостоке')
        assert response.intent.startswith(intent.GetWeather)
        assert 'Сейчас во Владивостоке' in response.text

    def test_pogoda_drop_unknown_where(self, alice):
        response = alice('погода в выквырагтыргынгыне')
        assert response.intent.startswith(intent.GetWeather)
        self._assert_unknown_where(response.text, 'в выквырагтыргынгыне')

        response = alice('послезавтра')
        assert response.intent.startswith(intent.GetWeather)
        assert 'Послезавтра в Москве' in response.text  # switch to Moscow

    def test_pogoda_dont_drop_known_where(self, alice):
        response = alice('погода в рязани')
        assert response.intent.startswith(intent.GetWeather)
        assert 'Сейчас в Рязани' in response.text

        response = alice('послезавтра')
        assert response.intent.startswith(intent.GetWeather)
        assert 'Послезавтра в Рязани' in response.text  # don't switch to Moscow

    @pytest.mark.xfail(reason="https://st.yandex-team.ru/HOLLYWOOD-1027")
    def test_forecast_drop_unknown_where(self, alice):
        response = alice('будет ли дождь в мухосранске вечером')
        assert response.intent.startswith(intent.GetWeather)
        self._assert_unknown_where(response.text, 'в мухосранске')

        response = alice('а ночью будет')
        assert response.intent.startswith(intent.GetWeather)
        assert response.slots['day_part'].string.strip('"') == 'night'
        assert 'Москва' in response.slots['forecast_location'].string  # switch to Moscow

    def test_forecast_dont_drop_known_where(self, alice):
        response = alice('будет ли дождь в питере вечером')
        assert response.intent.startswith(intent.GetWeather)
        assert response.slots['day_part'].string.strip('"') == 'evening'
        assert 'Санкт-Петербург' in response.slots['forecast_location'].string

        response = alice('а ночью будет')
        assert response.intent.startswith(intent.GetWeather)
        assert response.slots['day_part'].string.strip('"') == 'night'
        assert 'Санкт-Петербург' in response.slots['forecast_location'].string  # don't switch to Moscow


@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.experiments('weather_use_cloud_ui')
@pytest.mark.supported_features('cloud_ui')
class TestPogodaCloudUi(PogodaDialogBase):

    owners = ('abc:weatherbackendvteam', )

    def test_open_uri(self, alice):
        response = alice('погода завтра')
        d = response.directive
        assert d.name == directives.names.OpenUriDirective
        assert d.payload.uri
        assert d.payload.screen_id == 'cloud_ui'


@pytest.mark.parametrize('surface', [surface.launcher])
class TestPogodaLegacy(object):

    owners = ('abc:weatherbackendvteam', )

    def test_today_weather_callback(self, alice):
        response = alice.update_weather()
        assert response.scenario == scenario.Weather
        assert response.intent == intent.GetWeather
