import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import voice
from alice.hollywood.library.python.testing.it2.stubber import HttpResponseStub
from alice.hollywood.library.scenarios.music.it2_music_client.music_sdk_helpers import (
    check_music_sdk_uri,
    check_response,
    check_suggests_common,
    get_response,
)


logger = logging.getLogger(__name__)


# BASE CLASSES
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.parametrize('surface', [surface.searchapp])
@pytest.mark.supported_features('music_sdk_client')
@pytest.mark.experiments('internal_music_player')
class _TestsSearchappUnauthorizedBase:

    @staticmethod
    def _assert_music_vertical_response(response):
        assert get_response(response).Layout.OutputSpeech in [
            'Попробуйте выбрать что-то из этого послушать.',
            'Давайте что-нибудь послушаем.',
        ]

        d = get_response(response).Layout.Directives
        assert len(d) == 1
        assert d[0].HasField('OpenUriDirective')
        assert d[0].OpenUriDirective.Name == 'music_vertical_show'
        assert d[0].OpenUriDirective.Uri == 'https://music.yandex.ru/pptouch'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments('station_promo_score=0')
class _TestsSearchappBase(_TestsSearchappUnauthorizedBase):
    pass


# AVAILABILITY
class _TestsSearchappAvailabilityBase(_TestsSearchappUnauthorizedBase):

    UNAUTHORIZED_TEXTS = [
        'Пожалуйста, войдите в аккаунт на Яндексе, чтобы я вас узнала и запомнила ваши вкусы.',
        'Пожалуйста, войдите в аккаунт, чтобы я могла ставить лайки вашим любимым песням.',
        'Войдите в аккаунт на Яндексе, и я включу вам то, что вы любите.',
    ]

    NO_PLUS_TEXTS = [
        'Хорошо, включу для вас небольшой отрывок, потому что без подписки иначе не получится. '
        'Кстати, вы можете оформить подписку сейчас и получить вместе с ней Станцию. На ней слушать музыку гораздо удобнее.',
    ]

    STATION_PROMO_TEXTS = [
        'Включаю. Хотя слушать музыку удобнее на Станции - и вы можете прямо сейчас получить '
        'ее, оформив специальную подписку.',
    ]

    @pytest.mark.parametrize('texts', [
        pytest.param(UNAUTHORIZED_TEXTS, id='unauthorized'),
        pytest.param(NO_PLUS_TEXTS, id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
        pytest.param(STATION_PROMO_TEXTS, id='station_promo', marks=[
            pytest.mark.experiments('station_promo_score=1'),
            pytest.mark.oauth(auth.YandexPlus)
        ]),
    ])
    def test_smoke(self, alice, texts):
        '''
        Добавится предложение войти в аккаунт или промо
        '''
        r = alice(voice('включи укупника'))
        assert r.scenario_stages() == {'run', 'continue'}

        layout = r.continue_response.ResponseBody.Layout
        assert layout.OutputSpeech == 'Включаю'

        assert len(layout.Cards) == 2
        assert layout.Cards[0].TextWithButtons.Text in texts
        assert layout.Cards[1].Div2CardExtended

        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-artist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'artist': ['1488960'],  # artist_id of "Аркадий Укупник"
        })

        return str(r)

    @pytest.mark.parametrize('dummy', [
        pytest.param(1, id='unauthorized'),
        pytest.param(2, id='no_plus', marks=pytest.mark.oauth(auth.Yandex)),
    ])
    def test_general_query(self, alice, dummy):
        '''
        Запрос "включи музыку" без плюса/неавторизованным должен открывать сайт Я.Музыки
        '''
        r = alice(voice('включи музыку'))
        self._assert_music_vertical_response(r)
        return str(r)

    def test_unauthorized_personal_query(self, alice):
        '''
        Запрос "включи мою музыку" для неавторизованных должен отвечать "Вы не авторизовались"
        '''
        r = alice(voice('включи мою музыку'))
        assert get_response(r).Layout.OutputSpeech == 'Вы не авторизовались.'

        cards = get_response(r).Layout.Cards
        assert len(cards) == 1
        assert cards[0].HasField('TextWithButtons')
        assert cards[0].TextWithButtons.Text == 'Вы не авторизовались.'

        buttons = cards[0].TextWithButtons.Buttons
        assert len(buttons) == 1
        assert buttons[0].Title == 'Авторизоваться'

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)

        return str(r)


@pytest.mark.skip
class TestsSearchappAvailabilityBass(_TestsSearchappAvailabilityBase):
    pass


class TestsSearchappAvailabilityHw(_TestsSearchappAvailabilityBase):
    pass


# ONDEMAND
class _TestsSearchappOndemandBase(_TestsSearchappBase):

    def test_track(self, alice):
        r = alice(voice('включи alter mann'))

        check_response(r, output_speeches=['Включаю'], with_div2_card=True)
        check_suggests_common(r, has_search_suggest=True)
        # XXX(sparkle): we check here only for track count, not for track order
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-track'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
        }, tracks_count=21, first_track=22769)

        return str(r)  # for debug purposes

    def test_artist(self, alice):
        r = alice(voice('включи rammstein'))

        check_response(r, output_speeches=['Включаю'], with_div2_card=True)
        check_suggests_common(r, has_search_suggest=True)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-artist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'artist': ['13002'],  # artist_id of "Rammstein"
        })

        return str(r)  # for debug purposes

    def test_album_from_multiple_artists(self, alice):
        r = alice(voice('включи альбом 50 шедевров классики'))

        check_response(r, output_speeches=['Включаю'], with_div2_card=True)
        check_suggests_common(r, has_search_suggest=True)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-album'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'album': ['2479018'],  # album_id of "50 SHEDEVROV KLASSIKI"
        })

        return str(r)  # for debug purposes

    def test_album_from_single_artist(self, alice):
        r = alice(voice('включи альбом mutter'))

        check_response(r, output_speeches=['Включаю'], with_div2_card=True)
        check_suggests_common(r, has_search_suggest=True)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-album'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'album': ['3542'],  # album_id of "Mutter"
        })

        return str(r)  # for debug purposes


@pytest.mark.skip
class TestsSearchappOndemandBass(_TestsSearchappOndemandBase):
    pass


class TestsSearchappOndemandHw(_TestsSearchappOndemandBase):
    pass


# PLAYLISTS
class _TestsSearchappPlaylistsBase(_TestsSearchappBase):

    @pytest.mark.parametrize('command, name, kind, owner', [
        # редакторские плейлисты
        pytest.param(
            'поставь плейлист вечные хиты',
            'Вечные хиты',
            '1250',
            '105590476',
            id='eternal_hits',
        ),
        pytest.param(
            'включи подборку лёгкое электронное утро',
            'Лёгкое электронное утро',
            '1459',
            '103372440',
            id='electro_morning',
        ),
        pytest.param(
            'включи громкие новинки месяца',
            'Громкие новинки месяца',
            '1175',
            '103372440',
            id='loud_novelty',
        ),

        # личные плейлисты
        pytest.param(
            'включи плейлист дня',
            'Плейлист дня',
            '127167070',
            '503646255',
            id='playlist_of_the_day',
        ),
        pytest.param(
            'запусти плейлист дежавю',
            'Дежавю',
            '119307282',
            '692528232',
            id='dejavu',
        ),
        pytest.param(
            'поставь плейлист премьера',
            'Премьера',
            '121818957',
            '692529388',
            id='premiere',
        ),
    ])
    def test_playlist(self, alice, command, name, kind, owner):
        r = alice(voice(command))

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': [kind],
            'owner': [owner],
        })

        return str(r)

    def test_playlist_origin(self, alice):
        r = alice(voice('включи плейлист с Алисой'))

        assert get_response(r).Layout.OutputSpeech == 'Я бы с радостью, но умею такое только в приложении Яндекс Музыка, или в умных колонках.'
        uri = get_response(r).Layout.Directives[0].OpenUriDirective.Uri
        assert uri == ('intent://playlist/origin/?from=alice&play=1#Intent;scheme=yandexmusic;package=ru.yandex.music;'
                       'S.browser_fallback_url=https%3A%2F%2Fmusic.yandex.ru%2Fplaylist%2Forigin%2F%3Ffrom%3Dalice%26mob%3D0%26play%3D1;end')

        return str(r)

    def test_my_music(self, alice):
        r = alice(voice('включи мою музыку'))

        assert get_response(r).Layout.OutputSpeech in [
            'Послушаем ваше любимое.',
            'Включаю ваши любимые песни.',
            'Окей. Плейлист с вашей любимой музыкой.',
            'Люблю песни, которые вы любите.',
            'Окей. Песни, которые вам понравились.',
        ]
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': ['3'],
            'owner': ['1083955728'],
        })

        return str(r)


@pytest.mark.skip
class TestsSearchappPlaylistsBass(_TestsSearchappPlaylistsBase):
    pass


class TestsSearchappPlaylistsHw(_TestsSearchappPlaylistsBase):
    pass


# RADIO
class _TestsSearchappRadioBase(_TestsSearchappBase):

    @pytest.mark.parametrize('command, kind, owner', [
        pytest.param('включи рэп', '1039', '414787002', id='genre'),
        pytest.param('включи музыку девяностых', '1122', '837761439', id='epoch'),
        pytest.param('включи грустную музыку', '1065', '837761439', id='mood'),
        pytest.param('включи музыку для бега', '1078', '837761439', id='activity'),
        pytest.param('включи итальянскую музыку', '1662', '103372440', id='language'),
        # pytest.param('включи инструментальную музыку', id='vocal'),  # TODO(sparkle): not working!
    ])
    def test_smoke(self, alice, command, kind, owner):
        r = alice(voice(command))

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': [kind],
            'owner': [owner],
        })

        return str(r)

    def test_missing_mapping(self, alice):
        # there is no mapping for "mood:cool"
        r = alice(voice('алиса включи крутую музыку'))

        output_speeches = [
            'Именно этой музыки сейчас нет, но попробуйте выбрать что-то из этого послушать.',
            'Хорошая музыка, но, к сожалению, недоступна. Давайте послушаем что-нибудь ещё.',
        ]

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        assert get_response(r).Layout.OutputSpeech in output_speeches

        d = get_response(r).Layout.Directives
        assert len(d) == 1
        assert d[0].HasField('OpenUriDirective')
        assert d[0].OpenUriDirective.Name == 'music_vertical_show'
        assert d[0].OpenUriDirective.Uri == 'https://music.yandex.ru/pptouch'

        cards = get_response(r).Layout.Cards
        assert len(cards) == 1
        assert cards[0].HasField('TextWithButtons')
        assert cards[0].TextWithButtons.Text in output_speeches

        buttons = cards[0].TextWithButtons.Buttons
        assert len(buttons) == 1
        assert buttons[0].Title == 'Перейти'

        return str(r)


@pytest.mark.skip
class TestsSearchappRadioBass(_TestsSearchappRadioBase):
    pass


class TestsSearchappRadioHw(_TestsSearchappRadioBase):
    pass


# MUSIC FLOW
class _TestsSearchappFlowBase(_TestsSearchappBase):

    def test_smoke(self, alice):
        '''
        На запрос "включи музыку" включается "плейлист дня"
        '''
        r = alice(voice('включи музыку'))

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': ['127167070'],
            'owner': ['503646255'],
        })

        return str(r)


@pytest.mark.skip
class TestsSearchappFlowBass(_TestsSearchappFlowBase):
    pass


class TestsSearchappFlowHw(_TestsSearchappFlowBase):
    pass


# FAIRY TALES
class _TestsSearchappFairyTalesBase(_TestsSearchappBase):

    def test_smoke(self, alice):
        r = alice(voice('включи сказку'))

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['true'],  # fairy tales are shuffling
            'kind': ['1039'],
            'owner': ['970829816'],
        })
        analytics_info = get_response(r).AnalyticsInfo
        assert analytics_info.Intent == 'personal_assistant.scenarios.music_fairy_tale'
        assert analytics_info.ProductScenarioName == 'music_fairy_tale'

        return str(r)


@pytest.mark.skip
class TestsSearchappFairyTalesBass(_TestsSearchappFairyTalesBase):
    pass


class TestsSearchappFairyTalesHw(_TestsSearchappFairyTalesBase):
    pass


# MISCELLANEOUS TESTS OUTSIDE OF GROUPS
class _TestsSearchappMiscBase(_TestsSearchappBase):

    @pytest.mark.freeze_stubs(bass_stubber={
        '/megamind/prepare': [
            HttpResponseStub(200, 'freeze_stubs/bass_megamind_prepare_music_not_found.json'),
        ],
    })
    def test_music_not_found(self, alice):
        r = alice(voice('алиса включи лермонтов три пальмы'))
        assert r.scenario_stages() == {'run'}
        assert r.run_response.ResponseBody.Layout.OutputSpeech in [
            'К сожалению, у меня нет такой музыки.',
            'Была ведь эта музыка у меня где-то... Не могу найти, простите.',
            'Как назло, именно этой музыки у меня нет.',
            'У меня нет такой музыки, попробуйте что-нибудь другое.',
            'Я не нашла музыки по вашему запросу, попробуйте ещё.',
        ]
        return str(r)

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/search': [
            HttpResponseStub(200, 'freeze_stubs/music_back_playlist_search_no_cover.json'),
        ],
    })
    def test_cover_doesnt_exist(self, alice):
        r = alice(voice('поставь плейлист вечные хиты'))
        return str(r)

    @pytest.mark.freeze_stubs(music_back_stubber={
        '/internal-api/playlists/personal/{playlist_id}': [
            HttpResponseStub(200, 'freeze_stubs/music_back_playlist_of_the_day_not_ready.json'),
        ],
    })
    def test_playlist_of_the_day_not_ready(self, alice):
        '''
        На запрос "включи музыку" должен был включаться "плейлист дня",
        но если он не готов ("ready": false), то надо как-то нормально ответить,
        для этого просто открываем сайт Я.Музыки (как в TestsSearchappAvailabilityBase.test_general_query)

        P.S. Этот кейс бывает только если юзер никогда не пользовался музыкой, т.е. ему
        надо просто зайти на Я.Музыку, если это ПП
        '''
        r = alice(voice('включи музыку'))
        self._assert_music_vertical_response(r)
        return str(r)


@pytest.mark.skip
class TestsSearchappMiscBass(_TestsSearchappMiscBase):
    pass


class TestsSearchappMiscHw(_TestsSearchappMiscBase):
    pass


# RUP STREAMS
class _TestsRupStreamsBase(_TestsSearchappBase):

    @pytest.mark.parametrize('command, kind, owner', [
        pytest.param('включи поток новое', '121818957', '692529388', id='recent_tracks'),
        pytest.param('включи поток забытое', '108185622', '460141773', id='missed_likes'),
        pytest.param('включи радиостанцию незнакомое', '119307282', '692528232', id='never_heard'),
        pytest.param('включи поток популярное', '1076', '414787002', id='special_chart'),
        pytest.param('включи поток любимое', '3', '1083955728', id='special_personal'),
        pytest.param('включи поток дня', '127167070', '503646255', id='special_onyourwave'),
    ])
    def test_smoke(self, alice, command, kind, owner):
        r = alice(voice(command))

        check_suggests_common(r, has_search_suggest=True, has_recommendation_suggests=False)
        check_music_sdk_uri(r, expected_query_info={
            'from': ['musicsdk-ru_yandex_searchplugin-alice-playlist'],
            'play': ['true'],
            'repeat': ['repeatOff'],
            'shuffle': ['false'],
            'kind': [kind],
            'owner': [owner],
        })

        return str(r)


@pytest.mark.experiments('hw_music_sdk_client_disable_search_app')
class TestsRupStreamsBass(_TestsRupStreamsBase):
    pass


@pytest.mark.experiments('hw_music_sdk_client_search_app')
class TestsRupStreamsHw(_TestsRupStreamsBase):
    pass
