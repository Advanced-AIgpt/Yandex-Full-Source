import re
from functools import partial

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.surface as surface
import pytest


def _assert_button(button, assert_uri_func):
    assert button
    expr, msg = assert_uri_func(button.directives[0].payload.uri)
    assert expr, msg


def _assert_open_uri(directive, assert_uri_func):
    assert directive.name == directives.names.OpenUriDirective
    expr, msg = assert_uri_func(directive.payload.uri)
    assert expr, msg


def _assert_musicsdk_uri(directive, assert_uri_func):
    def _check(uri):
        expr, msg = assert_uri_func(uri)
        expr2 = uri.startswith('musicsdk://') and 'play=true' in uri and expr
        msg2 = f'{msg}; {uri} does not start with `musicsdk://` or `play=true` not in {uri}'
        return expr2, msg2

    _assert_open_uri(directive, _check)


def _assert_text(alice, text, navi_pattern, searchapp_text):
    if surface.is_navi(alice):
        if isinstance(navi_pattern, str):
            assert navi_pattern in text
        else:
            assert navi_pattern.search(text) is not None
    elif surface.is_searchapp(alice):
        assert searchapp_text in text
    else:
        assert False, 'Unknown surface'


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmSearchapp(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1824
    https://testpalm.yandex-team.ru/testcase/alice-1837
    https://testpalm.yandex-team.ru/testcase/alice-2038
    """

    owners = ('zhigan',)

    @pytest.mark.voice
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', ['включи музыку', 'посоветуй музыку', 'давай послушаем музыку'])
    def test_alice_1824(self, alice, command):
        response = alice(command)
        assert response.output_speech_text.startswith('Включаю') or response.output_speech_text.startswith('Открываю')
        assert len(response.cards) == 1
        assert response.div_card
        assert response.cards[0].type == 'div2_card'
        assert not response.text_card
        _assert_musicsdk_uri(response.directive, lambda uri: ('owner=503646255' in uri,
                                                              f'`owner=503646255` not in {uri}'))

    @pytest.mark.no_oauth
    def test_alice_1837(self, alice):
        response = alice('Включи мою музыку')
        assert response.text.startswith('Вы не авторизовались')
        _assert_button(response.button('Авторизоваться'), lambda uri: (uri == 'yandex-auth://?theme=light',
                                                                       f'{uri} != yandex-auth://?theme=light'))

        login_responses = (
            'Пожалуйста, войдите в аккаунт на Яндексе, чтобы я вас узнала и запомнила ваши вкусы.',
            'Пожалуйста, войдите в аккаунт, чтобы я могла ставить лайки вашим любимым песням.',
            'Войдите в аккаунт на Яндексе, и я включу вам то, что вы любите.',
        )

        # login_responses_fairytale = (
        #     'Пожалуйста, войдите в свой аккаунт на Яндексе, чтобы я могла включать вам сказки целиком. А пока послушайте отрывок.',
        #     'Пожалуйста, войдите в аккаунт, чтобы я могла включать вам сказки, которые вы любите, полностью. А пока - отрывок.',
        # )

        response = alice('Включи Rammstein')
        assert response.text.startswith(login_responses)
        _assert_button(response.button('Войти'), lambda uri: (uri == 'yandex-auth://?theme=light',
                                                              f'{uri} != yandex-auth://?theme=light'))
        _assert_musicsdk_uri(response.directive, lambda uri: ('artist=13002' in uri,
                                                              f'`artist=13002` not in {uri}'))

        response = alice('Включи сказку о рыбаке и рыбке')
        # XFAIL https://st.yandex-team.ru/ALICEINFRA-955
        # assert response.text.startswith(login_responses_fairytale) and 'Включаю "Сказка о рыбаке и рыбке"' in response.text, response.text
        _assert_button(response.button('Войти'), lambda uri: (uri == 'yandex-auth://?theme=light',
                                                              f'{uri} != yandex-auth://?theme=light'))
        _assert_musicsdk_uri(response.directive, lambda uri: (any([
            track_id in uri for track_id in ['track=27736044', 'track=740614']]),
            f'`track=27736044` or `track=740614` not in {uri}'))

        response = alice('Включи радио дача')
        assert response.text.startswith(login_responses) and '"Радио Дача"' in response.text
        _assert_button(response.button('Войти'), lambda uri: (uri == 'yandex-auth://?theme=light',
                                                              f'{uri} != yandex-auth://?theme=light'))
        _assert_open_uri(response.directive, lambda uri: (uri == 'http://music.yandex.ru/fm/radio_dacha',
                                                          f'{uri} != http://music.yandex.ru/fm/radio_dacha'))

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.version(hollywood=146)
    def test_alice_2038(self, alice):
        response = alice('Включи Мадонну')
        assert response.text == ('Хорошо, включу для вас небольшой отрывок, потому что без подписки иначе не получится. '
                                 'Кстати, вы можете оформить подписку сейчас и получить вместе с ней Станцию. '
                                 'На ней слушать музыку гораздо удобнее.')

        expected_uri = 'https://plus.yandex.ru/station-lite?utm_source=pp&utm_medium=dialog_alice&utm_campaign=MSCAMP-24|lite'
        _assert_button(response.button('Подробнее'),
                       lambda uri: (uri == expected_uri, f'{uri} != {expected_uri}'))
        _assert_musicsdk_uri(response.directive, lambda uri: ('artist=1813' in uri,
                                                              f'`artist=1813` not in {uri}'))


@pytest.mark.parametrize('surface', [surface.navi])
class TestPalmNavi(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2516
    https://testpalm.yandex-team.ru/testcase/alice-2517
    """

    owners = ('vitvlkv',)

    @pytest.mark.no_oauth
    @pytest.mark.parametrize('command', [
        'Включи музыку',
        'Поставь Rammstein',
    ])
    def test_alice_2516(self, alice, command):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        assert response.text in [
            'Если хотите послушать музыку, авторизуйтесь, пожалуйста.',
            'Авторизуйтесь, пожалуйста, и сразу включу вам все, что попросите!',
        ]
        assert not response.directive

    @pytest.mark.oauth(auth.Yandex)
    def test_alice_2517(self, alice):
        response = alice('Включи музыку')
        assert response.intent == intent.MusicPlay
        assert response.text in [
            'Чтобы слушать музыку, вам нужно оформить подписку Яндекс.Плюс.',
            'Извините, но я пока не могу включить то, что вы просите. Для этого необходимо оформить подписку на Плюс.',
            'Простите, я бы с радостью, но у вас нет подписки на Плюс.',
        ]
        assert not response.directive


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.navi, surface.searchapp])
class TestPalmInternalMusicPlayer(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1825
    https://testpalm.yandex-team.ru/testcase/alice-1826
    https://testpalm.yandex-team.ru/testcase/alice-1827
    https://testpalm.yandex-team.ru/testcase/alice-1828
    https://testpalm.yandex-team.ru/testcase/alice-1829
    https://testpalm.yandex-team.ru/testcase/alice-1830
    """

    owners = ('vitvlkv',)

    @pytest.mark.parametrize('command, first_track_pattern', [
        ('Вруби трек show must go on', re.compile(r'Show Must Go On')),
        ('Включи песню облади облада', re.compile(r'Ob-La-Di, Ob-La-Da')),
        ('Поставь трек выхода нет сплин', re.compile(r'Сплин.*Выхода нет')),
        ('Запусти варвара группы би-2', re.compile(r'Би-2.*Варвара')),
    ])
    def test_alice_1825(self, alice, command, first_track_pattern):
        response = alice(command)
        assert response.intent == intent.MusicPlay
        if surface.is_searchapp(alice):
            assert response.output_speech_text == 'Включаю'
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            _assert_text(alice, response.text, first_track_pattern, 'Включаю')
        _assert_musicsdk_uri(response.directive, lambda uri: ('track=' in uri,
                                                              f'`track=` not in {uri}'))

    @pytest.mark.parametrize('command, album_name, album_ids', [
        ('Включи the dark side of the moon', 'Pink Floyd, альбом "The Dark Side Of The Moon"', [297567]),
        ('Включи гранатовый альбом', 'Сплин, альбом "Гранатовый альбом"', [60058]),
    ])
    def test_alice_1826(self, alice, command, album_name, album_ids):
        response = alice(command)

        assert response.intent == intent.MusicPlay
        if surface.is_searchapp(alice):
            assert response.output_speech_text == 'Включаю'
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            _assert_text(alice, response.text, f'Включаю: {album_name}.', 'Включаю альбом')

        def assert_uri(uri, album_ids):
            for album_id in album_ids:
                if f'album={album_id}' in uri:
                    return True, ''
            return False, f'`None of album_ids {album_ids}` found in {uri}'

        _assert_musicsdk_uri(response.directive, partial(assert_uri, album_ids=album_ids))

    @pytest.mark.parametrize('command, artist_name, artist_id', [
        ('Поставь скорпионс', 'Scorpions', 90),
        ('Вруби король и шут', 'Король и Шут', 41052),
    ])
    def test_alice_1827(self, alice, command, artist_name, artist_id):
        response = alice(command)

        assert response.intent == intent.MusicPlay
        if surface.is_searchapp(alice):
            assert response.output_speech_text == 'Включаю'
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            _assert_text(alice, response.text, f'Включаю: {artist_name}.', 'Включаю исполнителя')
        _assert_musicsdk_uri(response.directive, lambda uri: (f'artist={artist_id}' in uri,
                                                              f'`artist={artist_id}` not in {uri}'))

    @pytest.mark.parametrize('command, playlist_name', [
        # alice-1828
        ('Поставь плейлист вечные хиты', 'Вечные хиты'),
        ('Включи подборку лёгкое электронное утро', 'Лёгкое электронное утро'),
        ('Включи громкие новинки месяца', 'Громкие новинки месяца'),
        # alice-1830
        ('Включи плейлист дня', 'Плейлист дня'),
        ('Запусти плейлист дежавю', 'Дежавю'),
        ('Поставь плейлист премьера', 'Премьера'),
    ])
    def test_alice_1828_1830(self, alice, command, playlist_name):
        # TODO(a-square): check uid and kind values if they're stable
        response = alice(command)

        assert response.intent == intent.MusicPlay
        if surface.is_searchapp(alice):
            assert response.output_speech_text == 'Включаю'
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            _assert_text(alice, response.text, f'Включаю подборку "{playlist_name}".', 'Включаю плейлист')
        _assert_musicsdk_uri(response.directive, lambda uri: ('kind' in uri and 'owner' in uri,
                                                              f'`kind` or `owner` (or both) not in {uri}'))

    @pytest.mark.parametrize('command', [
        # alice-1829
        ('Поставь грустную музыку'),
        ('Включи музыку для вечеринки'),
    ])
    def test_alice_1829(self, alice, command):
        response = alice(command)

        assert response.intent == intent.MusicPlay
        if surface.is_searchapp(alice):
            assert response.output_speech_text == 'Включаю'
            assert len(response.cards) == 1
            assert response.div_card
            assert response.cards[0].type == 'div2_card'
            assert not response.text_card
        else:
            _assert_text(alice, response.text, 'Включаю', 'Включаю плейлист')
        if surface.is_navi(alice):
            _assert_musicsdk_uri(response.directive, lambda uri: ('radio' in uri and 'tag' in uri,
                                                                  f'`radio` or `tag` (or both) not in {uri}'))
        else:
            _assert_musicsdk_uri(response.directive, lambda uri: ('kind' in uri and 'owner' in uri,
                                                                  f'`kind` or `owner` (or both) not in {uri}'))
