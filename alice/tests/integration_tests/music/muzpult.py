import alice.megamind.protos.common.frame_pb2 as frame_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from music.generative import EXPERIMENTS_THIN_CLIENT_GENERATIVE
from music.thin_client import assert_audio_play_directive


EMusicPlayType = frame_pb2.TMusicPlayObjectTypeSlot.EValue


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestMuzpult(object):

    owners = ('zhigan', 'amullanurov')

    @pytest.mark.parametrize('object_id, object_type, track_id', [
        pytest.param('139086', EMusicPlayType.Track, '139086', id='track'),
        pytest.param('4066489', EMusicPlayType.Album, '33274668', id='album'),
        pytest.param('3121', EMusicPlayType.Artist, None, id='artist'),
        pytest.param('105590476:1250', EMusicPlayType.Playlist, '2758009', id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, None, id='radio'),
    ])
    def test_music_play(self, muzpult, object_id, object_type, track_id):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3255
        https://testpalm.yandex-team.ru/testcase/alice-3256
        https://testpalm.yandex-team.ru/testcase/alice-3257
        https://testpalm.yandex-team.ru/testcase/alice-3259
        https://testpalm.yandex-team.ru/testcase/alice-3260
        """
        response = muzpult.play(object_id, object_type)
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

        if track_id:
            assert response.directive.payload.stream.id == track_id

    @pytest.mark.parametrize('object_id, object_type, start_from_track_id, offset_sec', [
        pytest.param('139086', EMusicPlayType.Track, None, 60, id='track'),
        pytest.param('4066489', EMusicPlayType.Album, '33274668', 60, id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', 60, id='artist'),
        pytest.param('178190693:1044', EMusicPlayType.Playlist, '718429', 60, id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, '39479', 60, id='radio'),
    ])
    def test_start_from_track_id_and_offset(self, muzpult, object_id, object_type, start_from_track_id, offset_sec):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3275
        https://testpalm.yandex-team.ru/testcase/alice-3277
        https://testpalm.yandex-team.ru/testcase/alice-3282
        https://testpalm.yandex-team.ru/testcase/alice-3322
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=start_from_track_id, offset_sec=offset_sec)
        assert response.scenario == scenario.HollywoodMusic
        assert response.directive.name == directives.names.AudioPlayDirective
        assert not response.text

        if start_from_track_id:
            assert response.directive.payload.stream.id == start_from_track_id


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(*EXPERIMENTS_THIN_CLIENT_GENERATIVE)
@pytest.mark.parametrize('surface', [surface.loudspeaker, surface.station])
class TestMuzpultThin(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('object_id, object_type, track_id, title, artist', [
        pytest.param('139086', EMusicPlayType.Track, None, 'Starlight', 'Muse', id='track'),
        pytest.param('4066489', EMusicPlayType.Album, '33274668', 'I. Dogma', 'Mick Gordon', id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', 'Прыгну со скалы', 'Король и Шут', id='artist'),
        pytest.param('105590476:1250', EMusicPlayType.Playlist, None, 'The Show Must Go On', 'Queen', id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, '39479', 'The Unforgiven II', 'Metallica', id='radio'),
        pytest.param('generative:lucky', EMusicPlayType.Generative, None, 'Мне повезёт!', None, id='generative'),
    ])
    def test_audio_play(self, muzpult, object_id, object_type, track_id, title, artist):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3255
        https://testpalm.yandex-team.ru/testcase/alice-3256
        https://testpalm.yandex-team.ru/testcase/alice-3257
        https://testpalm.yandex-team.ru/testcase/alice-3259
        https://testpalm.yandex-team.ru/testcase/alice-3260
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=track_id)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.text
        assert_audio_play_directive(response.directive, title=title, subtitle=artist)

        assert response.directive.payload.stream.id
        if track_id:
            assert response.directive.payload.stream.id == track_id

        music_metadata = response.directive.payload.metadata.glagol_metadata.music_metadata
        assert music_metadata.id == object_id
        assert music_metadata.type == EMusicPlayType.Name(object_type)

    @pytest.mark.parametrize('object_id, object_type, track_id, title, prev_title', [
        pytest.param('139086', EMusicPlayType.Track, None, 'Starlight', None, id='track'),
        pytest.param('7019257', EMusicPlayType.Album, '50685846', 'Прыгну со скалы', 'Бедняжка', id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', 'Прыгну со скалы', 'Лесник', id='artist'),
        pytest.param('178190693:1044', EMusicPlayType.Playlist, '718429', 'Quiero Saber', 'Break Stuff', id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, '39479', 'The Unforgiven II', None, id='radio'),
        pytest.param('generative:lucky', EMusicPlayType.Generative, None, 'Мне повезёт!', None, id='generative'),
    ])
    def test_prev(self, muzpult, object_id, object_type, track_id, title, prev_title):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3272
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=track_id)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.text
        assert_audio_play_directive(response.directive, title=title)

        response = muzpult.play_prev()
        assert response.scenario == scenario.HollywoodMusic
        if prev_title is None:
            assert response.text in [
                'Извините, я не запомнила, какой трек был предыдущим.',
                'Простите, я отвлеклась и не запомнила, что играло до этого.',
                'Простите, я совершенно забыла, что включала до этого.',
                'Извините, но я выходила во время предыдущего трека и не знаю, что играло.',
            ]
        else:
            assert response.intent == intent.PlayPreviousTrack
            assert not response.text
            assert_audio_play_directive(response.directive, title=prev_title)

    @pytest.mark.parametrize('object_id, object_type, track_id, title, next_title', [
        pytest.param('139086', EMusicPlayType.Track, None, 'Starlight', None, id='track'),
        pytest.param('7019257', EMusicPlayType.Album, '50685846', 'Прыгну со скалы', 'Девушка и Граф', id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', 'Прыгну со скалы', 'Дурак и молния', id='artist'),
        pytest.param('178190693:1044', EMusicPlayType.Playlist, '718429', 'Quiero Saber', 'Woman Is Still a Woman', id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, '39479', 'The Unforgiven II', None, id='radio'),
        pytest.param('generative:lucky', EMusicPlayType.Generative, None, 'Мне повезёт!', 'Мне повезёт!', id='generative'),
    ])
    def test_next(self, muzpult, alice, object_id, object_type, track_id, title, next_title):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3272
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=track_id)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.text
        assert_audio_play_directive(response.directive, title=title)

        response = muzpult.play_next()
        assert response.intent == intent.PlayNextTrack
        assert not response.text
        if object_type != EMusicPlayType.Generative:
            assert_audio_play_directive(response.directive, title=next_title)

    @pytest.mark.parametrize('object_id, object_type, start_from_track_id, offset_sec, title', [
        pytest.param('139086', EMusicPlayType.Track, None, 60, 'Starlight', id='track'),
        pytest.param('4066489', EMusicPlayType.Album, '33274668', 60, 'I. Dogma', id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', 60, 'Прыгну со скалы', id='artist'),
        pytest.param('178190693:1044', EMusicPlayType.Playlist, '718429', 60, 'Quiero Saber', id='playlist'),
    ])
    def test_start_from_track_id_and_offset(self, muzpult, object_id, object_type, start_from_track_id, offset_sec, title):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3275
        https://testpalm.yandex-team.ru/testcase/alice-3277
        https://testpalm.yandex-team.ru/testcase/alice-3282
        https://testpalm.yandex-team.ru/testcase/alice-3322
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=start_from_track_id, offset_sec=offset_sec)
        assert response.scenario == scenario.HollywoodMusic
        assert response.intent == intent.MusicPlay
        assert not response.text
        assert_audio_play_directive(response.directive, title=title, offset_ms=offset_sec * 1000)

        assert response.directive.payload.stream.id
        if start_from_track_id:
            assert response.directive.payload.stream.id == start_from_track_id

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.parametrize('object_id, object_type, track_id', [
        pytest.param('139086', EMusicPlayType.Track, None, id='track'),
        pytest.param('4066489', EMusicPlayType.Album, '33274668', id='album'),
        pytest.param('41052', EMusicPlayType.Artist, '50685846', id='artist'),
        pytest.param('105590476:1250', EMusicPlayType.Playlist, None, id='playlist'),
        pytest.param('genre:metal', EMusicPlayType.Radio, '39479', id='radio'),
        pytest.param('generative:lucky', EMusicPlayType.Generative, None, id='generative'),
    ])
    def test_audio_play_without_plus(self, muzpult, object_id, object_type, track_id):
        """
        https://testpalm.yandex-team.ru/testcase/alice-3284
        """
        response = muzpult.play(object_id, object_type, start_from_track_id=track_id)
        assert response.scenario == scenario.HollywoodMusic
        assert not response.directive
        assert response.text in [
            'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
            'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
            'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
        ]
