import alice.tests.library.surface as surface
import alice.tests.library.scenario as scenario
import alice.tests.library.directives as directives
import pytest

from urllib.parse import quote


@pytest.mark.experiments('mm_enable_protocol_scenario=MapsDownloadOffline', 'bg_fresh_granet=alice.maps.download_offline')
class _TestBase(object):

    owners = ('abc:mobilemaps',)

    uri_scheme_maps = 'yandexmaps://maps.yandex.ru/offline-maps'
    uri_scheme_maps_intent = 'intent://yandex.ru/maps/offline-maps'
    uri_search_by_name = 'search_by=name'
    uri_search_by_span = 'search_by=current_camera_position'
    uri_name = 'name='
    uri_start_download = 'start_download=true'
    uri_city = quote('Город')


@pytest.mark.parametrize('surface', [surface.maps])
class TestDownloadOfflineMaps(_TestBase):

    response_variants = {
        'Минутку...',
        'Один момент...',
        'Смотрю, что у нас есть...',
    }

    @pytest.mark.parametrize('command', [
        'Скачай карту de_dust2',
        'Загрузи карту Пятёрочки',
        'Сохрани карту маршрута Москва Питер',
    ])
    def test_scenario_miss(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.MapsDownloadOffline

    @pytest.mark.parametrize('command', [
        'Скачай карту видимой области',
        'Загрузи эту карту',
        'Сохрани карту',
    ])
    def test_download_by_span(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.text in self.response_variants
        assert response.directive.name == directives.names.OpenUriDirective
        uri = response.directive.payload.uri
        assert self.uri_scheme_maps in uri
        assert self.uri_search_by_span in uri
        assert self.uri_start_download in uri

    @pytest.mark.parametrize('command', [
        'Скачай карту Москвы',
        'Загрузи карту Питера',
        'Сохрани карту Ленобласти',
    ])
    def test_download_by_name(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.text in self.response_variants
        assert response.directive.name == directives.names.OpenUriDirective
        uri = response.directive.payload.uri
        assert self.uri_scheme_maps in uri
        assert self.uri_name in uri
        assert self.uri_search_by_name in uri
        assert self.uri_start_download in uri

    # 'город' is captured by GeoAddr in grnt
    @pytest.mark.parametrize('command', [
        'Скачай карту города Москва',
        'Загрузи карту города СПб',
    ])
    def test_city_prefix_is_removed(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.directive.name == directives.names.OpenUriDirective
        assert self.uri_city not in response.directive.payload.uri

    # https://st.yandex-team.ru/MAPSPRODUCT-1623#6149d043031cf97a10120966
    @pytest.mark.parametrize('command', [
        'Скачай карту Франции',
        'Загрузи карту Риги для работы офлайн',
    ])
    def test_difficult_names(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestDownloadOfflineSearchApp(_TestBase):

    response_variants = {
        'Открываю Яндекс.Карты.',
    }

    @pytest.mark.parametrize('command', [
        'Скачай карту видимой области',
        'Загрузи эту карту',
    ])
    def test_download_by_span(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.text in self.response_variants
        assert response.directive.name == directives.names.OpenUriDirective
        uri = response.directive.payload.uri
        assert self.uri_scheme_maps_intent in uri
        assert self.uri_search_by_span in uri
        assert self.uri_start_download in uri

    @pytest.mark.parametrize('command', [
        'Скачай карту Москвы',
        'Загрузи карту Питера',
    ])
    def test_download_by_name(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.text in self.response_variants
        assert response.directive.name == directives.names.OpenUriDirective
        uri = response.directive.payload.uri
        assert self.uri_scheme_maps_intent in uri
        assert self.uri_name in uri
        assert self.uri_search_by_name in uri
        assert self.uri_start_download in uri


@pytest.mark.parametrize('surface', [surface.station])
class TestDownloadOfflineStation(_TestBase):

    response_variants = {
        'К сожалению, я здесь не помогу. Загрузить офлайн-карты можно в последней версии приложения Яндекс.Карты.',
        'Боюсь, здесь я не помогу. Попробуйте в Яндекс.Картах — только нужна последняя версия приложения.',
    }

    @pytest.mark.parametrize('command', [
        'Скачай карту видимой области',
        'Загрузи карту Москвы',
    ])
    def test_download(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MapsDownloadOffline
        assert response.text in self.response_variants
        assert not response.directive
