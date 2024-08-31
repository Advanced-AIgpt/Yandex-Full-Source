import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('change_track')
@pytest.mark.parametrize('surface', [surface.station])
class TestPlayVideoWithLang(object):

    owners = ('amullanurov',)

    def test_play_video_english_language(self, alice):
        response = alice('включи рик и морти на английском')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'

    def test_play_video_english_subtitles(self, alice):
        response = alice('включи рик и морти с английскими субтитрами')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.subtitles_language == 'eng'

    def test_play_video_english_language_and_russian_subtitles(self, alice):
        response = alice('включи рик и морти на английском языке с русскими субтитрами')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.audio_language == 'eng'
        assert response.directive.payload.subtitles_language == 'rus'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('change_track', 'show_video_settings')
@pytest.mark.parametrize('surface', [surface.station_pro])
class TestPlayVideoWithSurroundedTracks(object):

    owners = ('amullanurov',)

    def test_play_video_bladerunner_2049(self, alice):
        response = alice('включи бегущий по лезвию 2049')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert any(audio_track.language == 'rus-x-51' for audio_track in response.directive.payload.item.audio_streams)
