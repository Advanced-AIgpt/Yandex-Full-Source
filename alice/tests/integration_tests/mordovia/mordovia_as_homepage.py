import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestMordoviaAsHomePage(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1296
    https://testpalm2.yandex-team.ru/testcase/alice-1298
    """

    owners = ('akormushkin',)

    def _check_go_home(self, response):
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directives[0].name == directives.names.MordoviaShowDirective
        assert '/video/quasar/home/' in response.directives[0].payload.url
        assert response.directives[0].payload.go_back

        assert response.directives[-1].name == directives.names.ClearQueueDirective

    @pytest.mark.parametrize('command', ['домой'])
    def test_go_home(self, alice, command):
        response = alice(command)
        self._check_go_home(response)

    @pytest.mark.parametrize('command1', ['Включи канал РБК', 'Включи муз тв'])
    @pytest.mark.parametrize('command2', ['Домой', 'На домашний экран'])
    def test_alice_1298(self, alice, command1, command2):  # go home from TV-player
        response = alice(command1)
        assert response.directive.name == directives.names.VideoPlayDirective

        response = alice(command2)
        self._check_go_home(response)

    @pytest.mark.parametrize('command', ['Домой', 'На домашний экран'])
    def test_alice_1296_native(self, alice, command):  # go home from native TV-gallery
        response = alice('Что по тв?')
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        response = alice(command)
        self._check_go_home(response)

    @pytest.mark.experiments('tv_channels_webview')
    @pytest.mark.parametrize('command', ['Домой', 'На домашний экран'])
    def test_alice_1296_webview(self, alice, command):  # go home from webview TV-gallery
        response = alice('Что по тв?')
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/channels' in response.directive.payload.url

        response = alice(command)
        self._check_go_home(response)
