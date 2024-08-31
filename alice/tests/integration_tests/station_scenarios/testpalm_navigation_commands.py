import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestNavigation(object):

    owners = ('akormushkin',)

    def _test_go_forward_and_back(self, alice):
        response = alice('Дальше')
        assert response.directive.name == directives.names.GoForwardDirective

        response = alice('Назад')
        assert response.directive.name == directives.names.GoBackwardDirective

    def _test_go_to_the_end_and_back(self, alice):
        response = alice('В конец')
        assert response.directive.name == directives.names.GoToTheEndDirective

        response = alice('В начало')
        assert response.directive.name == directives.names.GoToTheBeginningDirective

    def test_alice_1039_native(self, alice):
        response = alice('Что по тв')
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        self._test_go_forward_and_back(alice)

    @pytest.mark.experiments('tv_channels_webview')
    def test_alice_1039_webview(self, alice):
        response = alice('Что по тв')
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/channels' in response.directive.payload.url

        self._test_go_forward_and_back(alice)

    def test_alice_1053_native(self, alice):
        response = alice('Что по тв')
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        self._test_go_to_the_end_and_back(alice)

    @pytest.mark.experiments('tv_channels_webview')
    def test_alice_1053_webview(self, alice):
        response = alice('Что по тв')
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/channels' in response.directive.payload.url

        self._test_go_to_the_end_and_back(alice)
