import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.parametrize('surface', [surface.station])
class TestPlayVideo(object):

    owners = ('akormushkin',)

    def test_next_video(self, alice):
        response = alice('включи следующее')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.VideoPlayDirective

    def test_previous_video(self, alice):
        response = alice('включи предыдущее')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.PlayPreviousTrack
        assert response.directive.name == directives.names.VideoPlayDirective

    _video_item = {
        'available': 1,
        'description': 'Диета по группе крови.  Как питаться и похудеть при 3 и 4 группах крови.',
        'provider_info': [{
            'available': 1,
            'provider_item_id': 'http://ok.ru/video/483977990633',
            'provider_name': 'yavideo',
            'type': 'video',
        }],
        'provider_item_id': 'http://ok.ru/video/483977990633',
        'provider_name': 'yavideo',
        'type': 'video',
        'name': 'Диета по группе крови. Как питаться и похудеть при 3 и 4 группах крови',
    }

    device_state = {
        'video': {
            'current_screen': 'video_player',
            'currently_playing': {
                'next_item': _video_item,
                'item': {
                    'available': 1,
                    'previous_items': [_video_item],
                    'description': 'http://dieta.trepel.ru/ Эксперимент канала. В последнее время все чаще идут разговоры о диете по группе крови.',
                    'provider_item_id': 'adNapX2G8NU',
                    'play_uri': 'youtube://adNapX2G8NU',
                    'thumbnail_url_16x9_small': 'https://avatars.mds.yandex.net/get-vthumb/366886/4342694b0a5f0ebeaba65818decc56d4/800x360',
                    'next_items': [_video_item],
                    'source': 'video_source_yavideo',
                    'provider_name': 'youtube',
                    'type': 'video',
                    'name': 'Диета по группе крови и в чем ее секрет',
                }
            }
        }
    }
