import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest
from library.python import resource


@pytest.mark.parametrize('surface', [surface.station])
class TestUpDownNavigation(object):

    owners = ('akormushkin',)
    device_state = resource.find('mordovia_video_selection_device_state.json')

    @pytest.mark.parametrize('command', ['выше', 'алиса выше', 'ещё выше', 'вверх'])
    def test_go_up(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.GoUpDirective

    @pytest.mark.parametrize('command', ['ниже', 'алиса ниже', 'ещё ниже', 'вниз'])
    def test_go_down(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.GoDownDirective

    @pytest.mark.parametrize('command', ['наверх', 'на самый верх', 'алиса в самый вверх'])
    def test_go_top(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.GoTopDirective


class TestSupportUpDownNavigation(object):

    owners = ('akormushkin',)
    commands = ['выше', 'алиса ниже', 'на самый верх']
    navigation_intents = {intent.GoDown, intent.GoTop, intent.GoUp}
    navigation_screens = ['mordovia_webview', 'search_results', 'content_details']

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('surface', [surface.station])
    def test_wrong_screen(self, alice, command):
        alice('включи фильм лалалэнд')
        response = alice(command)
        assert response.intent not in self.navigation_intents

        alice('включи радио')
        response = alice(command)
        assert response.intent not in self.navigation_intents

        alice('включи музыку')
        response = alice(command)
        assert response.intent not in self.navigation_intents

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('command', commands)
    @pytest.mark.parametrize('current_screen', navigation_screens)
    @pytest.mark.parametrize('surface', surface.actual_surfaces)
    def test_surface_and_screen(self, alice, current_screen, command):
        alice.device_state.Video.CurrentScreen = current_screen
        response = alice(command)
        if surface.is_quasar(alice) or 'vertical_screen_navigation' in alice.supported_features:
            assert response.intent in self.navigation_intents
        else:
            assert response.intent not in self.navigation_intents
