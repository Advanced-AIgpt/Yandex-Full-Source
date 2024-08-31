import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest
from library.python import resource


_last_watched_item = {
    'movies': [
        {
            # Один дома (номер 7)
            'provider_item_id': '4ed0391f9e10d314aa0a7de2ea07bf55',
            'provider_name': 'kinopoisk',
            'timestamp': 1593172305,
            'audio_language': 'rus',
            'subtitles_language': 'eng'
        },
        {
            # Достать ножи
            'provider_item_id': '411a4de4ea461ddf943dec0dfd29afc1',
            'provider_name': 'kinopoisk',
            'timestamp': 1593172305,
            'audio_language': 'eng',
            'subtitles_language': 'eng'
        }

    ],
    'tv_shows': [
        {
            # Рик и Морти
            'item': {
                'episode': 1,
                'provider_item_id': '4b7e75f953b028e5b856bbaf23d5459f',
                'provider_name': 'kinopoisk',
                'season': 1,
                'timestamp': 1593167655,
                'audio_language': 'eng',
                'subtitles_language': 'rus'
            },
            'tv_show_item': {
                'progress': {
                    'duration': 1270,
                    'played': 626
                },
                'provider_item_id': '46c5df252dc1a790b82d1a00fcf44812',
                'provider_name': 'kinopoisk',
                'timestamp': 1593167655,
                'audio_language': 'eng',
                'subtitles_language': 'rus'
            }
        }
    ],
    'videos': []
}


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.device_state(last_watched=_last_watched_item)
@pytest.mark.parametrize('surface', [surface.station])
class TestRecoveryTracksInMordovia(object):

    owners = ('amullanurov', )
    device_state = resource.find('mordovia_video_selection_device_state.json')

    def test_recovery_tracks_movie(self, alice):
        response = alice('включи 7')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'rus'
        assert response.directive.payload.subtitles_language == 'eng'

    def test_recovery_tracks_serie(self, alice):
        response = alice('включи рик и морти')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'rus'

        response = alice('следующая серия')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'rus'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.device_state(last_watched=_last_watched_item)
@pytest.mark.parametrize('surface', [surface.station])
class TestRecoveryTracksInVideoBass(object):

    owners = ('amullanurov', )

    def test_recovery_tracks_movie(self, alice):
        response = alice('включи достать ножи')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'eng'

    def test_recovery_tracks_serie(self, alice):
        response = alice('включи рик и морти')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'rus'

        response = alice('следующая серия')
        assert response.intent == intent.PlayNextTrack
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'rus'
