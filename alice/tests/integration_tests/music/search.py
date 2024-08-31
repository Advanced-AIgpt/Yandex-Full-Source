import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


def _check_text(response, entries):
    if not isinstance(entries, (list, tuple)):
        entries = [entries]

    assert 'Включаю' in response.text
    for entry in entries:
        if isinstance(entry, str):
            assert entry in response.text
        else:
            assert re.search(entry, response.text)


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.dexp])
@pytest.mark.experiments('music_force_show_first_track')
class TestPalmMusicSearch(object):
    '''
    https://testpalm.yandex-team.ru/testcase/alice-469
    https://testpalm.yandex-team.ru/testcase/alice-470
    https://testpalm.yandex-team.ru/testcase/alice-471
    https://testpalm.yandex-team.ru/testcase/alice-472
    '''

    owners = ('vitvlkv', )

    @pytest.mark.parametrize('command_prefix', [
        'включи', 'запусти', 'врубай', 'поставь', 'давай послушаем',
    ])
    @pytest.mark.parametrize('command_object, response_snippet, first_track_title, answer_type', [
        # alice-471
        pytest.param(
            'красно желтые дни',
            'Красно-жёлтые дни',
            'КИНО, Красно-жёлтые дни',
            'Track',
            id='track_implicit_name',
        ),
        pytest.param(
            'трек show must go on',
            'The Show Must Go On',
            'Queen, The Show Must Go On',
            'Track',
            id='track_explicit_name',
        ),
        pytest.param(
            'песню pressure группы muse',
            'Pressure',
            'Muse, Pressure',
            'Track',
            id='track_explicit_artist_and_name',
        ),
        pytest.param(
            'crazy группы aerosmith',
            'Crazy',
            'Aerosmith, Crazy',
            'Track',
            id='track_implicit_artist_and_name',
        ),
        pytest.param(
            'агата кристи провода',
            'Ни там ни тут',
            'Агата Кристи, Ни там ни тут',
            'Track',
            id='track_by_artist_and_lyric',
        ),
        # alice-469
        pytest.param(
            'the beatles',
            'The Beatles',
            'The Beatles',
            'Artist',
            id='artist_implicit_name_eng',
        ),
        pytest.param(
            'группу the castaways',
            'The Castaways',
            'The Castaways',
            'Artist',
            id='artist_explicit_name_eng',
        ),
        pytest.param(
            'чайф',
            'ЧайФ',
            'ЧайФ',
            'Artist',
            id='artist_implicit_name_rus',
        ),
        pytest.param(
            'группу танцы минус',
            'Танцы Минус',
            'Танцы Минус',
            'Artist',
            id='artist_explicit_name_rus',
        ),
        # alice-470
        pytest.param(
            'dark side of the moon',
            ['Pink Floyd', 'The Dark Side Of The Moon'],
            'Pink Floyd',
            'Album',
            id='album_implicit_name',
        ),
        pytest.param(
            'альбом dark side of the moon',
            ['Pink Floyd', 'The Dark Side Of The Moon'],
            'Pink Floyd',
            'Album',
            id='album_explicit_name',
        ),
        # alice-472
        pytest.param(
            'музыкальные новинки',
            ['подборку', 'Громкие новинки месяца'],
            None,
            'Playlist',
            id='playlist_new',
        ),
        pytest.param(
            'плейлист рассвет',
            ['подборку', 'Рассвет'],
            None,
            'Playlist',
            id='playlist_sunrise',
        ),
        pytest.param(
            'подборку величайшие песни о любви',
            ['подборку', 'Величайшие песни о любви'],
            None,
            'Playlist',
            id='playlist_greatest_love_hits',
        ),
        pytest.param(
            'популярную музыку',
            ['подборку', 'Чарт'],
            None,
            'Playlist',
            id='playlist_popular',
        ),
        pytest.param(
            'современную музыку',
            ['подборку', re.compile(r'Громкие новинки месяца|Чарт')],  # XXX(ardulat): has to be recognized as "novelty"
            None,
            'Playlist',
            id='playlist_modern',
        ),
    ])
    def test_entity_search(self, alice, command_prefix, command_object, response_snippet, first_track_title, answer_type):
        response = alice(f'{command_prefix} {command_object}')

        # check that the expected scenario handled the query and that the station should now play music
        assert response.intent == intent.MusicPlay
        assert response.directive.name == directives.names.MusicPlayDirective

        # check that a matching music event occured
        music_event = response.scenario_analytics_info.event('music_event')
        assert music_event
        assert music_event.answer_type == answer_type

        # check that a matching first track started playing
        first_track = response.scenario_analytics_info.objects.get('music.first_track_id')
        assert first_track

        _check_text(response, response_snippet)

        if first_track_title is not None:
            assert first_track_title in first_track.human_readable
