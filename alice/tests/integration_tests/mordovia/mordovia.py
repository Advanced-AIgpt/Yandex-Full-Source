import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.experiments('ether')
class TestMordoviaShowMainScreen(object):

    owners = ('akormushkin',)

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command', ['что посмотреть'])
    def test_show_main_screen(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/home/' in response.directive.payload.url

    @pytest.mark.parametrize('command', ['что посмотреть'])
    @pytest.mark.parametrize('surface', [
        surface.aliceapp,
        surface.automotive,
        surface.dexp,
        surface.launcher,
        surface.loudspeaker,
        surface.navi,
        surface.searchapp,
        pytest.param(
            surface.smart_tv,
            marks=pytest.mark.xfail(reason='see TVANDROID-3881')
        ),
        surface.watch,
        surface.yabro_win
    ])
    def test_do_not_show_main_screen(self, alice, command):
        response = alice(command)
        assert response.scenario != scenario.MordoviaVideoSelection
        if response.directive:
            assert not (response.directive.name == directives.names.MordoviaShowDirective and ('/video/quasar/home/' in response.directive.payload.url))

    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command', ['включи яндекс эфир', 'покажи эфир'])
    def test_do_not_show_main_screen_as_ether(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.ShowTvGalleryDirective
