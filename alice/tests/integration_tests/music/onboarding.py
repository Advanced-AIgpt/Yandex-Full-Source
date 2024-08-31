import re

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


TRACKS_GAME_LIKE_PHRASES = [
    'Ура! Я старалась! Послушайте теперь этот.',
    'Отлично! Послушайте ещё вот этот.',
    'Очень рада! А как насчет этого?',
]
TRACKS_GAME_DISLIKE_PHRASES = re.compile(
    r'(Поняла.|Запомнила!|Очень жаль.) '
    r'(Послушайте теперь этот.|Попробуйте теперь вот этот.|А как насчет этого\?)'
)


def assert_play(audio_play_directive, pause=False):
    assert audio_play_directive.name == directives.names.AudioPlayDirective
    assert audio_play_directive.payload.set_pause == pause
    assert audio_play_directive.payload.stream.id


def assert_play_radio(response, pause=False, radio='user:onyourwave'):
    assert response.directives
    audio_play_directive = response.get_directive(directives.names.AudioPlayDirective)
    assert audio_play_directive

    assert_play(audio_play_directive, pause)
    assert audio_play_directive.payload.metadata.glagol_metadata.music_metadata.id == radio


def assert_music_intent(response, intent):
    assert response.scenario == scenario.HollywoodMusic
    assert response.intent == intent


def assert_complex_like_dislike(response, speech):
    assert_music_intent(response, intent.MusicComplexLikeDislike)
    assert response.text == speech


def assert_not_onboarding(response):
    assert_music_intent(response, intent.MusicPlay)
    assert_play(response.directive)


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(
    'bg_alice_music_like',
    'bg_alice_music_onboarding',
    'bg_alice_music_onboarding_artists',
    'bg_alice_music_onboarding_genres',
    'bg_alice_music_onboarding_tracks',
    'hw_music_onboarding',
    'hw_music_complex_like',
)
@pytest.mark.parametrize('surface', [surface.station(is_tv_plugged_in=False)])
class _TestsOnboardingBase(object):
    owners = ('flimsywhimsy', 'deemonasd')


class TestsOnboardingArtists(_TestsOnboardingBase):

    @pytest.mark.parametrize('subcommand, output_speech, reask', [
        pytest.param('Radiohead', 'Radiohead, отличный выбор!', False, id='artist'),
        pytest.param('мне нравится Radiohead', 'Radiohead, отличный выбор!', False, id='like_artist'),
        pytest.param('мне не нравится Radiohead', 'Хорошо, поставила дизлайк. А какой исполнитель вам нравится?', True, id='dislike_artist'),
        pytest.param('песня Gold Guns Girls', 'Metric, отличный выбор!', False, id='artist_from_song'),
        pytest.param('мне нравится песня Gold Guns Girls', 'Metric, отличный выбор!', False, id='like_artist_from_song'),
        pytest.param('рок', 'Извините, я не смогла разобрать исполнителя. Можете повторить?', True, id='genre'),
        pytest.param('мне нравится рок', 'Извините, я не смогла разобрать исполнителя. Можете повторить?', True, id='like_genre'),
    ])
    def test_normal(self, alice, subcommand, output_speech, reask):
        response = alice('угадай исполнителей которые мне нравятся')
        assert_music_intent(response, intent.MusicOnboardingArtists)
        assert response.text == 'Кто ваш любимый исполнитель?'

        response = alice(subcommand)
        assert_complex_like_dislike(response, output_speech)

        if reask:
            response = alice('Radiohead')
            assert_complex_like_dislike(response, 'Radiohead, отличный выбор!')

        response = alice('Radiohead')
        assert_not_onboarding(response)


class TestsOnboardingGenres(_TestsOnboardingBase):

    @pytest.mark.parametrize('subcommand, output_speech, reask', [
        pytest.param('рок', 'Рок, мне нравится!', False, id='genre'),
        pytest.param('мне нравится рок', 'Рок, мне нравится!', False, id='like_genre'),
        pytest.param('Radiohead', 'Не могу найти такой жанр. Можете повторить?', True, id='artist'),
        pytest.param('мне нравится Radiohead', 'Не могу найти такой жанр. Можете повторить?', True, id='like_artist'),
    ])
    def test_normal(self, alice, subcommand, output_speech, reask):
        response = alice('угадай мои любимые музыкальные жанры')
        assert_music_intent(response, intent.MusicOnboardingGenres)
        assert response.text == 'Музыку какого жанра вы слушаете чаще всего?'

        response = alice(subcommand)
        assert_complex_like_dislike(response, output_speech)

        if reask:
            response = alice('рок')
            assert_complex_like_dislike(response, 'Рок, мне нравится!')

        response = alice('рок')
        assert_not_onboarding(response)


@pytest.mark.xfail(reason='https://st.yandex-team.ru/DIALOG-8822')
class TestsOnboardingTracks(_TestsOnboardingBase):

    @pytest.mark.experiments('hw_music_onboarding_tracks_count=4')
    def test_normal(self, alice):
        response = alice('угадай песни которые мне нравятся')
        assert_music_intent(response, intent.MusicOnboardingTracks)
        assert response.text == 'Хорошо! Включаю первый трек, а вы говорите, нравится или нет.'
        assert_play_radio(response)

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text in TRACKS_GAME_LIKE_PHRASES
        assert_play_radio(response)

        response = alice('дизлайк')
        assert response.intent == intent.PlayerDislike
        assert TRACKS_GAME_DISLIKE_PHRASES.fullmatch(response.text)
        assert_play_radio(response)

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text == 'Хорошо, попробуем последний.'
        assert_play_radio(response)

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text == 'Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?'
        assert_play_radio(response, pause=True)

        response = alice('нет')
        assert_music_intent(response, intent.MusicOnboardingTracks)
        assert response.text == 'Хорошо. Вы можете в любое время сказать мне "Алиса, включи музыку".'

    @pytest.mark.experiments('hw_music_onboarding_tracks_count=1')
    def test_normal_with_play_after(self, alice):
        response = alice('угадай песни которые мне нравятся')
        assert_music_intent(response, intent.MusicOnboardingTracks)
        assert response.text == 'Хорошо! Включаю первый трек, а вы говорите, нравится или нет.'
        assert_play_radio(response)

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text == 'Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?'
        assert_play_radio(response, pause=True)

        response = alice('давай')
        assert response.intent == intent.PlayerContinue
        assert_play_radio(response)


class TestsOutOfOnboardingLikeDislike(_TestsOnboardingBase):

    @pytest.mark.parametrize('command, output_speech', [
        pytest.param('мне нравится Radiohead', 'Radiohead, отличный выбор!', id='like_artist'),
        pytest.param('мне не нравится Radiohead', 'Хорошо, поставила дизлайк исполнителю.', id='dislike_artist'),
        pytest.param('мне нравится песня Gold Guns Girls', 'Хорошо, поставила лайк треку!', id='like_song'),
        pytest.param('мне не нравится песня Gold Guns Girls', 'Хорошо, поставила дизлайк треку.', id='dislike_song'),
        pytest.param('мне нравится альбом OK Computer', 'Хорошо, поставила лайк альбому!', id='like_album'),
        pytest.param('мне нравится рок', 'Рок, мне нравится!', id='like_genre'),
    ])
    def test_normal(self, alice, command, output_speech):
        response = alice(command)
        assert_music_intent(response, intent.MusicComplexLikeDislike)
        assert response.text == output_speech

    @pytest.mark.parametrize('command', [
        pytest.param('мне не нравится альбом OK Computer', id='dislike_album'),
        pytest.param('мне не нравится рок', id='dislike_genre'),
    ])
    def test_unsupported(self, alice, command):
        response = alice(command)
        assert_complex_like_dislike(response, 'Извините, я пока умею ставить дизлайки только песням и исполнителям.')

    # Should just go to music
    @pytest.mark.parametrize('command', [
        pytest.param('Radiohead', id='artist'),
        pytest.param('песня Gold Guns Girls', id='song'),
        pytest.param('рок', id='genre'),
    ])
    def test_no_like_dislike(self, alice, command):
        response = alice(command)
        assert_not_onboarding(response)


class TestsMasterOnboarding(_TestsOnboardingBase):

    @pytest.mark.parametrize('command, output_speech', [
        pytest.param('мне нравится Radiohead', 'Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?', id='like_artist'),
        pytest.param('мне не нравится Radiohead', 'Хорошо, поставила дизлайк исполнителю. А музыку какого жанра вы слушаете чаще всего?', id='dislike_artist'),
    ])
    def test_normal(self, alice, command, output_speech):
        response = alice('настрой музыкальные рекомендации')
        assert_music_intent(response, intent.MusicOnboarding)
        assert response.text == 'Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?'

        response = alice('джаз')
        assert_complex_like_dislike(response, 'Извините, я не смогла разобрать исполнителя. Можете повторить?')

        response = alice(command)
        assert_complex_like_dislike(response, output_speech)

        response = alice('рок')
        assert_complex_like_dislike(response, 'Рок, мне нравится! Давайте поиграем? Я вам включу несколько песен, а вы скажете, нравятся они вам или нет.')

        response = alice('нет')
        assert_music_intent(response, intent.MusicOnboardingTracks)
        assert response.text == 'Хорошо.'

        response = alice('джаз')
        assert_not_onboarding(response)

    @pytest.mark.experiments('hw_music_onboarding_genre_radio')
    def test_normal_with_tracks(self, alice):
        response = alice('настрой музыкальные рекомендации')
        assert_music_intent(response, intent.MusicOnboarding)
        assert response.text == 'Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?'

        response = alice('мне нравится Radiohead')
        assert_complex_like_dislike(response, 'Radiohead, отличный выбор! А музыку какого жанра вы слушаете чаще всего?')

        response = alice('рок')
        assert_complex_like_dislike(response, 'Рок, мне нравится! Давайте поиграем? Я вам включу несколько песен, а вы скажете, нравятся они вам или нет.')

        response = alice('давай')
        assert_music_intent(response, intent.MusicOnboardingTracks)
        assert response.text == 'Хорошо! Включаю первый трек.'
        assert_play_radio(response, radio='genre:rock')

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text in TRACKS_GAME_LIKE_PHRASES
        assert_play_radio(response, radio='genre:rock')

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text == 'Хорошо, попробуем последний.'
        assert_play_radio(response, radio='genre:rock')

        response = alice('лайк')
        assert response.intent == intent.PlayerLike
        assert response.text == 'Окей, запомнила. Теперь когда вы будете просить музыку я буду основываться на ваших предпочтениях. Давайте послушаем?'
        assert_play_radio(response, pause=True, radio='genre:rock')

        response = alice('давай')
        assert response.intent == intent.PlayerContinue
        assert_play_radio(response, radio='genre:rock')

        response = alice('джаз')
        assert_not_onboarding(response)

    def test_drop_from_onboarding_on_new_session(self, alice):
        response = alice('хочу настроить свои рекомендации в музыке')
        assert_music_intent(response, intent.MusicOnboarding)
        assert response.text == 'Для начала давайте узнаем друг друга лучше. Кто ваш любимый исполнитель?'

        response = alice('хватит')
        assert response.scenario != scenario.HollywoodMusic

        response = alice('давай послушаем музыку')
        assert_not_onboarding(response)

        response = alice('Radiohead')
        assert response.intent != intent.MusicComplexLikeDislike
