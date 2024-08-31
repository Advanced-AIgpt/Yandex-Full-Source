import pytest

import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface


@pytest.mark.version(hollywood=160)
@pytest.mark.experiments('use_app_host_pure_Video_scenario')
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestShowView(object):

    owners = ('eshamax',)

    def test_show_view_youtube(self, alice):
        response = alice('включи лесного медведя на youtube')
        assert response.scenario == scenario.Video
        assert len(response.directives) == 2
        assert any(directive.name == directives.names.ShowViewDirective for directive in response.directives)
        assert any(directive.name == directives.update_space_actions for directive in response.directives)
