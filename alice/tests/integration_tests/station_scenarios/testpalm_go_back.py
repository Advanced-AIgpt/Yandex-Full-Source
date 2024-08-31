import alice.tests.library.directives as directives
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestNavigationCommand(object):

    @pytest.mark.parametrize('command', ['назад'])
    def test_go_back(self, alice, command):
        alice('найди видео про самолеты')
        response = alice(command)
        assert response.directive.name == directives.names.GoBackwardDirective
