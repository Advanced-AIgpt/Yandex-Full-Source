import alice.tests.library.auth as auth
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from music.thin_client import assert_response, assert_audio_play_directive


EXPERIMENTS_CHANGE_TRACK_VERSION = [
    'hw_music_change_track_version',
    'bg_fresh_granet',
]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(*EXPERIMENTS_CHANGE_TRACK_VERSION)
@pytest.mark.parametrize('surface', [surface.station])
class TestChangeTrackVersion(object):

    owners = ('amullanurov',)

    def test_change_track_version(self, alice):
        response = alice('включи we will rock you')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen', title='We Will Rock You')
        first_track_id = response.directive.payload.metadata.glagol_metadata.music_metadata.id

        response = alice('включи другую версию')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle_re='((?!Queen).)*', title='We Will Rock You')
        assert first_track_id != response.directive.payload.metadata.glagol_metadata.music_metadata.id

    def test_change_track_version_remix(self, alice):
        response = alice('включи we will rock you')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen', title='We Will Rock You')
        first_track_id = response.directive.payload.metadata.glagol_metadata.music_metadata.id

        response = alice('включи ремикс')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen', title='We Will Rock You')
        assert first_track_id != response.directive.payload.metadata.glagol_metadata.music_metadata.id

    def test_change_track_version_live(self, alice):
        response = alice('включи we will rock you')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen', title='We Will Rock You')
        first_track_id = response.directive.payload.metadata.glagol_metadata.music_metadata.id

        response = alice('включи лайв')
        assert_response(response, sub_text='Включаю: ', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, subtitle='Queen', title='We Will Rock You')
        assert first_track_id != response.directive.payload.metadata.glagol_metadata.music_metadata.id
