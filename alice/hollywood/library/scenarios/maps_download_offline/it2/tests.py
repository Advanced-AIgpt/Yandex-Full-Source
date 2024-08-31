import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.experiments('bg_fresh_granet=alice.maps.download_offline')
@pytest.mark.scenario(name='MapsDownloadOffline', handle='maps_download_offline')
class _TestBase:

    def check_frame(self, response, expected):
        frame = response.run_response.ResponseBody.SemanticFrame.Name
        assert frame == expected

    def check_speech(self, response, expected):
        output_speech = response.run_response.ResponseBody.Layout.OutputSpeech
        assert output_speech in expected

    def check_uri(self, response, expected):
        directives = response.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].HasField('OpenUriDirective')
        open_uri_directive = directives[0].OpenUriDirective
        assert open_uri_directive.Uri == expected

    def check_no_uri(self, response):
        directives = response.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 0

    def check_relevant(self, response):
        assert not response.run_response.Features.IsIrrelevant

    def check_irrelevant(self, response):
        assert response.run_response.Features.IsIrrelevant


@pytest.mark.parametrize('surface', [surface.maps])
class TestOfflineMaps(_TestBase):

    speech_variants = {
        'Минутку...',
        'Один момент...',
        'Смотрю, что у нас есть...',
    }

    def test_spb(self, alice):
        r = alice(voice('скачай карту спб'))
        self.check_frame(r, 'alice.maps.download_offline')
        self.check_speech(r, TestOfflineMaps.speech_variants)
        self.check_uri(r, 'yandexmaps://maps.yandex.ru/offline-maps?name=%D0%A1%D0%B0%D0%BD%D0%BA%D1%82-%D0%9F%D0%B5%D1%82%D0%B5%D1%80%D0%B1%D1%83%D1%80%D0%B3&search_by=name&start_download=true')
        self.check_relevant(r)
        return str(r)

    def test_msk(self, alice):
        r = alice(voice('скачай карту города москва'))
        self.check_frame(r, 'alice.maps.download_offline')
        self.check_speech(r, TestOfflineMaps.speech_variants)
        self.check_uri(r, 'yandexmaps://maps.yandex.ru/offline-maps?name=%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0&search_by=name&start_download=true')
        self.check_relevant(r)
        return str(r)

    def test_current_span(self, alice):
        r = alice(voice('скачай эту карту'))
        self.check_frame(r, 'alice.maps.download_offline')
        self.check_speech(r, TestOfflineMaps.speech_variants)
        self.check_uri(r, 'yandexmaps://maps.yandex.ru/offline-maps?search_by=current_camera_position&start_download=true')
        self.check_relevant(r)
        return str(r)

    def test_current_span_uncertainly(self, alice):
        r = alice(voice('сохрани мне эту карту'))
        self.check_frame(r, 'alice.maps.download_offline_uncertainly')
        self.check_speech(r, TestOfflineMaps.speech_variants)
        self.check_uri(r, 'yandexmaps://maps.yandex.ru/offline-maps?search_by=current_camera_position&start_download=true')
        self.check_relevant(r)
        return str(r)


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestOpenIntent(_TestBase):

    speech_variants = {
        'Открываю Яндекс Карты.',
    }

    def test_msk(self, alice):
        r = alice(voice('скачай карту москвы'))
        self.check_speech(r, TestOpenIntent.speech_variants)
        self.check_uri(r, 'intent://yandex.ru/maps/offline-maps?name=%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0&search_by=name&start_download=true#Intent;scheme=yandexmaps;package=ru.yandex.yandexmaps;' +
                          'S.browser_fallback_url=https%3A%2F%2Fmobile.yandex.ru%2Fapps%2Fmaps;end')
        self.check_relevant(r)
        return str(r)

    def test_irrelevant(self, alice):
        r = alice(voice('сохрани карту'))
        self.check_speech(r, TestOpenIntent.speech_variants)
        self.check_uri(r, 'intent://yandex.ru/maps/offline-maps?search_by=current_camera_position&start_download=true#Intent;scheme=yandexmaps;package=ru.yandex.yandexmaps;' +
                          'S.browser_fallback_url=https%3A%2F%2Fmobile.yandex.ru%2Fapps%2Fmaps;end')
        self.check_irrelevant(r)
        return str(r)


@pytest.mark.parametrize('surface', [surface.station])
class TestStub(_TestBase):

    speech_variants = {
        'К сожалению, я здесь не помогу. Загрузить офлайн-карты можно в последней версии приложения Яндекс Карты.',
        'Боюсь, здесь я не помогу. Попробуйте в Яндекс Картах — только нужна последняя версия приложения.',
    }

    def test_msk(self, alice):
        r = alice(voice('скачай карту москвы'))
        self.check_speech(r, TestStub.speech_variants)
        self.check_no_uri(r)
        self.check_relevant(r)
        return str(r)

    def test_irrelevant(self, alice):
        r = alice(voice('сохрани карту'))
        self.check_speech(r, TestStub.speech_variants)
        self.check_no_uri(r)
        self.check_irrelevant(r)
        return str(r)
