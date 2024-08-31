import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
class TestPlaylists(object):

    owners = ('yagafarov',)

    @pytest.mark.parametrize('surface, playlist_id, text', [
        (surface.station_lite_purple, '103372440:2567', 'Включаю. Все, что нужно, чтобы без конца качало'),
        (surface.station_lite_green, '103372440:2568', 'Включаю. Настройтесь на дух Нью-Йорка шестидесятых'),
        (surface.station_lite_red, '103372440:2569', 'Включаю. В эфире все самое желанное'),
        (surface.station_lite_yellow, '103372440:2570', 'Включаю. Это должно добавить вам счастья'),
        (surface.station_lite_beige, '103372440:2571', 'Включаю. Никогда еще дома не было так хорошо'),
        (surface.station_lite_pink, '103372440:2572', 'Включаю. Добро пожаловать в волшебный мир музыки'),
    ])
    def test_hardcoded_lite_playlists(self, alice, playlist_id, text):
        response = alice('Включи свою любимую музыку')
        assert response.scenario == scenario.HollywoodHardcodedMusic
        assert response.has_voice_response()
        assert response.output_speech_text == text
        assert response.directive.name == directives.names.MusicPlayDirective
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event.answer_type == 'Playlist'
        assert music_event.id == playlist_id
