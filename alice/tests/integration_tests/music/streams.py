import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
class TestRupStreams(object):
    owners = ('mike88')

    @pytest.mark.parametrize('surface', [surface.dexp, surface.searchapp])
    @pytest.mark.parametrize('command, station_answer_type, playlist_kind', [
        ('включи поток новое', 'Filters', '121818957'),
        ('включи поток забытое', 'Filters', '108185622'),
        ('включи радиостанцию незнакомое', 'Filters', '119307282'),
        ('включи поток популярное', 'Filters', '1076'),
        ('включи мою волну', 'Filters', '127167070'),
        ('включи поток дня', 'Filters', '127167070'),
        ('включи поток лучшее', 'Filters', '127167070'),
        ('включи поток любимое', 'Filters', '3'),
    ])
    def test_rup_streams(self, alice, command, station_answer_type, playlist_kind):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        if surface.is_smart_speaker(alice):
            assert 'Включаю поток' in response.output_speech_text
            assert response.directive.name == directives.names.MusicPlayDirective
        else:
            assert response.directive.name == directives.names.OpenUriDirective
            assert f'kind={playlist_kind}' in response.directive.payload.uri
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        if surface.is_smart_speaker(alice):
            assert music_event.answer_type == station_answer_type
            if station_answer_type == 'Playlist':
                assert playlist_kind in music_event.id
        else:
            assert music_event.answer_type == 'Playlist'
            assert playlist_kind in music_event.id

    @pytest.mark.version(hollywood=152)
    @pytest.mark.parametrize('surface', [surface.station])
    @pytest.mark.parametrize('command, playlist_id, response_text', [
        ('включи мою волну', 'user:onyourwave', 'Включаю поток "Моя волна".'),
        ('включи поток новое', 'personal:recent-tracks', 'Включаю поток "Новое".'),
        ('включи поток забытое', 'personal:missed-likes', 'Включаю поток "Забытое".'),
        ('включи радиостанцию незнакомое', 'personal:never-heard', 'Включаю поток "Незнакомое".'),
        ('включи поток популярное', 'personal:hits', 'Включаю поток "Популярное".'),
        ('включи поток любимое', 'personal:collection', 'Включаю поток "Любимое".'),
    ])
    def test_rup_streams_thin(self, alice, command, playlist_id, response_text):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response_text in response.output_speech_text
        assert response.directive.name == directives.names.AudioPlayDirective
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.answer_type == 'Filters'
        assert playlist_id == music_event.id

    @pytest.mark.parametrize('surface', [surface.navi])
    @pytest.mark.parametrize('command, radio, tag', [
        ('включи мою волну', 'user', 'onyourwave'),
        ('включи поток незнакомое', 'personal', 'never-heard'),
        ('включи поток любимое', 'personal', 'collection'),
        ('поставь аудиопоток популярное', 'personal', 'hits'),
        ('включи станцию новинки', 'personal', 'recent-tracks'),
        ('включи аудиопоток флешбек', 'personal', 'missed-likes'),
    ])
    def test_rup_streams_navi(self, alice, command, radio, tag):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert response.output_speech_text == 'Включаю'
        assert response.directive.name == directives.names.OpenUriDirective
        assert f'radio={radio}' in response.directive.payload.uri
        assert f'tag={tag}' in response.directive.payload.uri
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event is not None
        assert music_event.answer_type == 'Filters'
