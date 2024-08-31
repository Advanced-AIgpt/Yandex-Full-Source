import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'video_disable_webview_video_entity',
    'video_disable_webview_video_entity_seasons',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestOpenCurrentVideo(object):

    owners = ('igor-darov',)

    def test_open_current_video(self, alice):
        response = alice('открой первый сезон доктора хауса')
        assert response.scenario == scenario.Video
        response = alice('включи')
        assert response.scenario == scenario.Vins
        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.text


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'video_pure_hw_content_details',
)
@pytest.mark.device_state(
    {
        "video": {
            "current_screen": "content_details",
            "tv_interface_state": {
                "content_details_screen": {
                    "current_item": {
                        "age_limit": 0,
                        "item_type": "movie",
                        "provider_item_id": "451faf4d11fb7fc5b2e1dc964b6e49fb",
                        "provider_name": "kinopoisk"
                    }
                }
            }
        }
    }
)
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestTvOpenCurrentVideo(object):

    owners = ('dandex',)

    def test_open_current_video(self, alice):
        response = alice('смотреть')
        assert response.scenario == scenario.Video
        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.text
