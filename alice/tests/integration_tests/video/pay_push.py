import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'video_disable_webview_video_entity',
    'video_disable_webview_video_entity_seasons',
)
@pytest.mark.parametrize('surface', [surface.station])
class TestPayPush(object):

    owners = ('akormushkin', )

    def test_play_with_pay(self, alice):
        alice('фильм агент ева')
        response = alice('смотреть')
        assert response.scenario == scenario.Video
        assert response.directive
        assert response.directive.name == directives.names.ShowPayPushScreenDirective

    @pytest.mark.parametrize('command', ['смотреть', 'включи', 'играй'])
    def test_play_without_pay(self, alice, command):
        alice('фильм джентльмены')
        response = alice(command)
        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name == 'Джентльмены'

    _last_watched_item = {
        'movies': [],
        'tv_shows': [],
        'videos': [
            {
                'play_uri': 'http://ok.ru/video/483977990633',
                'progress': {
                    'duration': 676,
                    'played': 666
                },
                'provider_item_id': 'http://ok.ru/video/483977990633',
                'provider_name': 'yavideo',
                'timestamp': 1548331766
            }
        ]
    }

    @pytest.mark.device_state(last_watched=_last_watched_item)
    def test_continue_watching(self, alice):
        alice('фильм богемская рапсодия')
        response = alice('продолжить смотреть')
        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
