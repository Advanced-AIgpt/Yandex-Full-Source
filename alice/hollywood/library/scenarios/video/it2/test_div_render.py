import pytest
from alice.hollywood.library.python.testing.it2 import surface
from alice.hollywood.library.python.testing.it2.input import voice


@pytest.mark.scenario(name='Video', handle='video')
@pytest.mark.parametrize('surface', [surface.smart_display])
class TestYouTube:
    def test_cats(self, alice):
        r = alice(voice('Включи котиков на ютубе'))
        layout = r.run_response.ResponseBody.Layout

        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('ShowViewDirective')

        action_spaces = r.run_response.ResponseBody.ActionSpaces
        assert len(action_spaces) == 1
        action_space_id = layout.Directives[0].ShowViewDirective.ActionSpaceId
        assert action_space_id in action_spaces
        assert len(action_spaces[action_space_id].Actions) == 2

        return str(layout.Directives[0].ShowViewDirective)


@pytest.mark.scenario(name='Video', handle='video')
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.experiments('centaur_video_search')
class TestCentaurVideoSearch:
    def test_video_search(self, alice):
        r = alice(voice('Найди фильмы с Папановым'))
        layout = r.run_response.ResponseBody.Layout

        assert len(layout.Directives) == 1
        assert layout.Directives[0].HasField('ShowViewDirective')

        return str(layout.Directives[0].ShowViewDirective)
