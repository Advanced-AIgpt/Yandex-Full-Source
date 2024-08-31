import alice.library.restriction_level.protos.content_settings_pb2 as content_settings_pb2
import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import json
import pytest


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station])
class _TestVideo(object):
    owners = ('akormushkin',)


class TestSearchInternetVideo(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-503
    """

    commands = [
        'Найди ролики в интернете',
        'Покажи ролики в сети',
        'Покажи видео про котиков',
        'Включи грустные видео про собак',
        'Найди смешные видео',
        pytest.param(
            'Найди +100500 на ютубе',
            marks=pytest.mark.xfail(reason='known issue, see VIDEOFUNC-750')
        ),
        'Включи This is хорошо на ютуб',
        'Найди фильм форрест гамп на youtube',
        'Покажи санта барбару',
        'Включи сериал скорая помощь'
    ]

    @pytest.mark.experiments('video_disable_webview_searchscreen')
    @pytest.mark.parametrize('command', commands)
    def test_native(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.ShowGalleryDirective

    @pytest.mark.parametrize('command', commands)
    def test_video_webview(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'videoSearch' in response.directive.payload.url


class _TestSearchKinopoiskContent(_TestVideo):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-665
    """

    def _run_test(self, alice, command):
        assert False, 'overload method _run_test'

    @pytest.mark.parametrize('command', [
        'Найди фильм про Терминатора',
        'Покажи кино Назад в будущее',
        pytest.param(
            'Найди фильм Воины',
            marks=pytest.mark.xfail(reason='Поиск отдаёт только один фильм с похожим названием на КП ("Воин")')
        ),
        'Найди фильм Звёздные Войны'
    ])
    def test_alice_373(self, alice, command):
        self._run_test(alice, command)

    @pytest.mark.parametrize('command', [
        'Найди комедии',
        'Покажи фильмы с Джонни Деппом',
        'Хочу фильмы 2010 года',
        'Найди фильмы с Морганом Фрименом',
        'Найди фильм про Гарри Поттера',
        'Включи комедии',
    ])
    def test_alice_664(self, alice, command):
        self._run_test(alice, command)

    @pytest.mark.parametrize('command', [
        'Найди смешные сериалы',
        'Найди сериалы 2014 года',
        'Найди сериалы с Натали Дормер',
        'Покажи сериалы с Сарой Джессикой Паркер'
    ])
    def test_alice_665(self, alice, command):
        self._run_test(alice, command)

    @pytest.mark.parametrize('command', ['Включи сериал доктор'])
    @pytest.mark.xfail(reason='Поиск отдаёт только один сериал с похожим названием на КП ("Хороший доктор")')
    def test_alice_646(self, alice, command):
        self._run_test(alice, command)


@pytest.mark.experiments(
    'video_disable_webview_searchscreen',
    'video_disable_films_webview_searchscreen',
    'video_disable_webview_use_ontoids',
)
class TestSearchKinopoiskContentNative(_TestSearchKinopoiskContent):
    def _run_test(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.ShowGalleryDirective
        assert response.directive.payload.items[0].provider_name == 'kinopoisk'


class TestSearchKinopoiskContentWebview(_TestSearchKinopoiskContent):
    def _run_test(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'filmsSearch' in response.directive.payload.url


class _TestItemSelection(_TestVideo):
    play_commands = ['Включи номер 3', 'Запусти второй']
    description_commands = ['Первый', 'Покажи описание номер 2']


@pytest.mark.experiments('video_disable_webview_searchscreen')
class TestVideoItemSelectionNative(_TestItemSelection):

    @pytest.mark.parametrize('command', _TestItemSelection.play_commands + _TestItemSelection.description_commands)
    def test_select_video_by_number(self, alice, command):
        alice('найди видео с котиками')
        response = alice(command)
        assert response.directive.name == directives.names.VideoPlayDirective

    @pytest.mark.parametrize('command', [
        'Включи номер 1000',
        pytest.param(
            'Удали будильник номер 2',
            marks=pytest.mark.xfail(reason='Will improve item selection quality in VAD-135 (see subtickets)')
        )
    ])
    def test_do_not_select_video(self, alice, command):
        alice('найди видео с котиками')
        response = alice(command)
        if response.directive:
            assert response.directive.name != directives.names.VideoPlayDirective


class TestTvShowSearch(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-383
    """

    @pytest.mark.parametrize('command', [
        'Найди сериал Крайний космос',
        'Найди Доктор Хаус',
        'Найди сериал Остаться в живых',
        'Найди Настоящий детектив',
    ])
    @pytest.mark.experiments('video_disable_webview_video_entity')
    def test_alice_383(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'


class TestTvShowSeasons(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-393
    """

    @pytest.mark.parametrize('command', [
        ('рик и морти 3 сезон', 'Рик и Морти', 3),
        ('Мир дикого запада новый сезон', 'Мир Дикого Запада', 4),
        ('Найди 5 сезон барбоскины', 'Барбоскины', 5),
        ('Список серий третьего сезона Настоящий детектив', 'Настоящий детектив', 3),
        ('Покажи все сезоны сериала Однажды в сказке', 'Однажды в сказке', 1),
    ])
    @pytest.mark.experiments('video_disable_webview_video_entity_seasons')
    def test_alice_393_show_season_gallery(self, alice, command):
        request, name, season = command
        response = alice(request)
        assert response.directive.name == directives.names.ShowSeasonGalleryDirective
        assert response.directive.payload.tv_show_item.provider_name == 'kinopoisk'
        assert response.directive.payload.tv_show_item.name == name
        assert response.directive.payload.season == season

    def test_alice_393_no_such_season(self, alice):
        response = alice('Покажи девятый сезон болотная тварь')
        assert response.text in ['Сезона с таким номером нет.', 'Такого сезона нет.']
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'videoEntity' in response.directive.payload.url

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_393_play(self, alice):
        response = alice('Включи 2 сезон доктора кто')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'
        assert response.directive.payload.tv_show_item.name == 'Доктор Кто'


class TestMoviePlay(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-407
    """

    def _check_native_gallery(self, response):
        assert response.directive.name == directives.names.ShowGalleryDirective
        assert response.directive.payload.items[0].provider_name == 'kinopoisk'
        assert response.directive.payload.items[0].name.startswith('Люди в черном')

    def _check_webview_gallery(self, alice, response):
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'filmsSearch' in response.directive.payload.url
        assert alice.device_state.Video.ViewState['sections'][0]['items'][0]['title'].startswith('Люди в чёрном')

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments(
        'video_disable_films_webview_searchscreen',
        'video_disable_webview_use_ontoids',
        'video_disable_webview_video_entity',
    )
    def test_alice_407(self, alice):
        response = alice('найди люди в черном')
        self._check_native_gallery(response)

        response = alice('включи номер 2')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name.startswith('Люди в черном')

        # FIXME: Should actually be 'назад' but it is not implemented yet.
        # response = alice('назад')
        response = alice('найди люди в черном')
        self._check_native_gallery(response)
        response = alice('включи люди в черном три')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name == 'Люди в черном 3'

        # FIXME: Should actually be 'назад' but it is not implemented yet.
        # response = alice('назад')
        response = alice('найди люди в черном')
        self._check_native_gallery(response)
        response = alice('первый')
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective
        assert response.directive.payload.item.name.startswith('Люди в черном')

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_407_webview(self, alice):
        response = alice('найди люди в черном')
        self._check_webview_gallery(alice, response)

        response = alice('включи номер 2')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name.startswith('Люди в черном')

        # FIXME: Should actually be 'назад' but it is not implemented yet.
        # response = alice('назад')
        response = alice('найди люди в черном')
        self._check_webview_gallery(alice, response)
        response = alice('включи люди в черном три')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name == 'Люди в черном 3'

        # FIXME: Should actually be 'назад' but it is not implemented yet.
        # response = alice('назад')
        response = alice('найди люди в черном')
        self._check_webview_gallery(alice, response)
        response = alice('первый')
        assert response.directive.name == directives.names.MordoviaCommandDirective
        assert 'videoEntity' in json.loads(response.directive.payload.meta)['path']
        # TODO(akormushkin) check item name here after VIDEOFUNC-885


class TestPaidMovieDescription(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-408
    """

    @pytest.mark.oauth(auth.Yandex)
    @pytest.mark.experiments(
        'video_disable_films_webview_searchscreen',
        'video_disable_webview_use_ontoids',
        'video_disable_webview_video_entity',
    )
    def test_alice_408(self, alice):
        response = alice('найди фильмы про бэтмена')
        assert response.directive.name == directives.names.ShowGalleryDirective
        assert response.directive.payload.items[0].provider_name == 'kinopoisk'

        response = alice('Включи номер 1')
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

        # see https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/common_nlg/video__common_ru.nlg#L175
        assert response.text.startswith((
            'Этот контент платный,',
            'Это платный контент,',
        ))
        assert response.text.endswith((
            'могу только открыть описание.',
            'сейчас открою описание.',
            'открываю описание.',
        ))
        # FIXME: There should be 'назад' command but it is not implemented yet.


class TestTvShowEpisodeNavigation(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-409
    """

    @pytest.mark.voice
    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.experiments('video_disable_webview_video_entity_seasons')
    def test_alice_409(self, alice):
        response = alice('второй сезон доктор хаус')
        assert response.directive.name == directives.names.ShowSeasonGalleryDirective
        assert response.directive.payload.items[0].provider_name == 'kinopoisk'
        assert response.directive.payload.season == 2
        assert response.directive.payload.tv_show_item.name == 'Доктор Хаус'

        response = alice('включи первую серию')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 2
        assert response.directive.payload.item.episode == 1
        assert response.directive.payload.item.name.endswith('Смирение')

        response = alice('Следующая серия')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 2
        assert response.directive.payload.item.episode == 2
        assert response.directive.payload.item.name.endswith('Аутопсия')

        response = alice('Давай последнюю серию')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 2
        assert response.directive.payload.item.episode == 24
        assert response.directive.payload.item.name.endswith('Без причины')

        response = alice('Давай первую серию')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 2
        assert response.directive.payload.item.episode == 1
        assert response.directive.payload.item.name.endswith('Смирение')

        response = alice('Включи первую серию сериала доктор хаус')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 1
        assert response.directive.payload.item.episode == 1
        assert response.directive.payload.item.name.endswith('Пилотная серия')

        response = alice('Предыдущая серия')
        assert response.directive.name == directives.names.ShowSeasonGalleryDirective
        assert response.directive.payload.items[0].provider_name == 'kinopoisk'
        assert response.directive.payload.season == 1
        assert response.directive.payload.tv_show_item.name == 'Доктор Хаус'
        assert response.text == 'Для этого видео нет предыдущего'

        response = alice('Включи последнюю серию сериала доктор хаус')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.season == 8
        assert response.directive.payload.item.episode == 22
        assert response.directive.payload.item.name.endswith('Все умирают')

        response = alice('Следующая серия')
        assert response.directive.name == directives.names.ShowSeasonGalleryDirective
        assert response.directive.payload.season == 8
        assert response.directive.payload.tv_show_item.name == 'Доктор Хаус'
        assert response.text == 'Для этого видео нет следующего'


@pytest.mark.experiments(
    'video_disable_webview_searchscreen',
    'video_disable_doc2doc',
)
class TestVideoNavigation(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-410
    """

    @pytest.mark.voice
    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_410(self, alice):
        response = alice('найди видео с енотиками')
        assert response.directive.name == directives.names.ShowGalleryDirective

        all_items = alice.device_state.Video.ScreenState.RawItems

        response = alice('включи номер 1')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['provider_item_id'] == all_items[0].ProviderItemId

        response = alice('следующее видео')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['provider_item_id'] == all_items[1].ProviderItemId

        response = alice('предыдущее видео')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['provider_item_id'] == all_items[0].ProviderItemId

        response = alice('Давай следующий ролик')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['provider_item_id'] == all_items[1].ProviderItemId

        response = alice('Вернись к предыдущему ролику')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['provider_item_id'] == all_items[0].ProviderItemId


class TestSearchCartoons(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-417
    """

    @pytest.mark.parametrize('command', [
        'Открой мультфильмы',
        'Открой мультфильмы союзмультфильм',
        'Включи мультики',
        'Включи мультики Диснея',
    ])
    def test_alice_417_search_kp(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.MordoviaShowDirective

    def test_alice_417_search_youtube(self, alice):
        response = alice('Открой мультфильмы на YouTube')
        assert response.directive.name == directives.names.MordoviaShowDirective

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', [
        'Включи мультфильм тайна коко',
        'Включи мультфильм Маша и медведь',
    ])
    def test_alice_417_search_kp_plus(self, alice, command):
        response = alice(command)
        assert response.directive.name == directives.names.VideoPlayDirective


class TestPornSearch(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-2250
    """

    owners = ('akormushkin',)

    @pytest.mark.parametrize('surface', [
        surface.station(device_config={'content_settings': content_settings_pb2.medium}),
        surface.station(device_config={'content_settings': content_settings_pb2.without}),
    ])
    def test_alice_2250_allowed(self, alice):
        response = alice('найди порно в сети')
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert 'videoSearch' in response.directive.payload.url

    @pytest.mark.parametrize('surface', [
        surface.station(device_config={'content_settings': content_settings_pb2.children}),
        surface.station(device_config={'content_settings': content_settings_pb2.safe}),
    ])
    def test_alice_2250_restricted(self, alice):
        response = alice('найди порно в сети')
        if response.directive:
            assert response.directive.name == directives.names.MordoviaShowDirective
            assert 'pf=strict' in response.directive.payload.url


@pytest.mark.parametrize('surface', [surface.watch])
class TestPalmVideoAtWatches(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-37
    """

    owners = ('tolyandex',)

    def test_alice_37(self, alice):
        response = alice('Найди фильм Последнее танго в париже')

        assert response.text in [
            'Извините, у меня нет хорошего ответа.',
            'У меня нет ответа на такой запрос.',
            'Я пока не умею отвечать на такие запросы.',
            'Простите, я не знаю что ответить.',
            'Я не могу на это ответить.',
            'По вашему запросу я ничего не нашла.',
            'По вашему запросу ничего не нашлось.',
            'По вашему запросу не получилось ничего найти.',
            'По вашему запросу ничего найти не получилось.',
            'К сожалению, я ничего не нашла.',
            'К сожалению, ничего не нашлось.',
            'К сожалению, не получилось ничего найти.',
            'К сожалению, ничего найти не получилось.',
            'Ничего не нашлось.',
            'Я ничего не нашла.',
            'Я ничего не смогла найти.',
        ]
        assert (response.intent == intent.Search or
                response.product_scenario == intent.ProtocolSearch)
        assert not response.directive


@pytest.mark.experiments(
    'video_disable_webview_searchscreen',
    'video_disable_doc2doc',
)
class TestVideoPlayerCommands(_TestVideo):
    """
    https://testpalm.yandex-team.ru/testcase/alice-404
    """

    @pytest.mark.oauth(auth.YandexPlus)
    def test_alice_404(self, alice):
        response = alice('найди видео с котиками')
        assert response.directive.name == directives.names.ShowGalleryDirective

        response = alice('включи номер 1')
        assert response.directive.name == directives.names.VideoPlayDirective

        response = alice('стоп')
        assert response.directive.name == directives.names.PlayerPauseDirective

        response = alice('Продолжить просмотр')
        assert response.directive.name == directives.names.PlayerContinueDirective

        response = alice('Останови видео')
        assert response.directive.name == directives.names.PlayerPauseDirective

        response = alice('Продолжить воспроизведение')
        assert response.directive.name == directives.names.PlayerContinueDirective

        response = alice('Пауза')
        assert response.directive.name == directives.names.PlayerPauseDirective

        response = alice('Играй')
        assert response.directive.name == directives.names.PlayerContinueDirective

        response = alice('Поставь на паузу')
        assert response.directive.name == directives.names.PlayerPauseDirective


@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
class TestVideoSearchWithoutScreen(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-555
    """

    expected_intents = [
        intent.VideoPlay,
        intent.Search,
        intent.Factoid,
        intent.ObjectAnswer,
        intent.ObjectSearchOO,
        intent.MusicPlay,
        intent.GeneralConversation,
    ]

    @pytest.mark.parametrize('command', [
        'Найди фильм про Терминатора',
        'Включи сериал Игра престолов',
        'Посоветуй хорошую комедию',
        'Включи последнюю серию Мира Дикого Запада',
        'Найди ролики на ютубе',
    ])
    def test_alice_555(self, alice, command):
        response = alice(command)
        if response.directive:
            assert response.directive.name != directives.names.VideoPlayDirective

        assert response.intent in self.expected_intents
        if response.intent == intent.VideoPlay:
            assert response.text in [
                'Чтобы смотреть видеоролики, фильмы и сериалы, нужно подключить Станцию к экрану.',
                'Чтобы смотреть видеоролики, фильмы и сериалы, подключите Станцию к экрану.',
                'Воспроизведение видео не поддерживается на этом устройстве.',
            ]

        if response.intent == intent.Search:
            assert response.text in [
                'Извините, у меня нет хорошего ответа.',
                'У меня нет ответа на такой запрос.',
                'Я пока не умею отвечать на такие запросы.',
                'Простите, я не знаю что ответить.',
                'Я не могу на это ответить.',
            ]

        if response.intent == intent.MusicPlay:
            assert response.directive.name == directives.names.MusicPlayDirective

        if response.intent == intent.GeneralConversation:
            assert response.text
