import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


# TODO(a-square): extend to all surfaces with internal music player
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('hollywood_music_play_anaphora', 'music_force_show_first_track')
class TestAnaphora(object):
    owners = ('vitvlkv', 'zhigan')

    device_state = {
        'music': {
            'currently_playing': {
                'track_info': {
                    'albums': [{
                        'id': 5829983
                    }],
                    'artists': [{
                        'id': 42528
                    }],
                    'id': '43741729',
                }
            }
        }
    }

    @pytest.mark.parametrize('surface', surface.smart_speakers)
    @pytest.mark.parametrize('command, track_title', [
        ('Включи этот альбом', 'Никто не хотел умирать'),
        ('Включи этого исполнителя', 'Гражданская оборона'),
        # ('Включи этот альбом в случайном порядке', 'Гражданская оборона'),  # XXX(a-square)
        ('Включи этого исполнителя в случайном порядке', 'Гражданская оборона'),
        # ('Включи этот альбом на репите', 'Никто не хотел умирать'),  # XXX(a-square)
        # ('Включи этого исполнителя на на репите', 'Гражданская оборона'),  # XXX(a-square)
    ])
    def test_anaphora_smart_speakers(self, alice, command, track_title):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text.startswith('Включаю')

        first_track = response.scenario_analytics_info.objects.get('music.first_track_id')
        assert track_title in first_track.human_readable

    # TODO(a-square): navi
    @pytest.mark.xfail(reason='https://st.yandex-team.ru/HOLLYWOOD-702')
    @pytest.mark.voice
    @pytest.mark.parametrize('surface', [surface.searchapp])
    @pytest.mark.parametrize('command, url_fragment', [
        ('Включи этот альбом', 'album=5829983'),
        ('Включи этого исполнителя', 'artist=42528'),
        # ('Включи этот альбом в случайном порядке', 'album=5829983'),  # XXX(a-square)
        ('Включи этого исполнителя в случайном порядке', 'artist=42528'),
        # ('Включи этот альбом на репите', 'Никто не хотел умирать'),  # XXX(a-square)
        # ('Включи этого исполнителя на на репите', 'Гражданская оборона'),  # XXX(a-square)
    ])
    def test_anaphora_internal_music_player(self, alice, command, url_fragment):
        response = alice(command)
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.OpenUriDirective
        assert url_fragment in response.directive.payload.uri
        assert response.output_speech_text.startswith('Включаю')
        assert len(response.cards) == 1
        assert response.div_card
        assert response.cards[0].type == 'div2_card'
        assert not response.text_card
