from urllib.parse import unquote

import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.region as region
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

import tv.div_card as div_card


@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPalmTvProgram(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1404
    https://testpalm.yandex-team.ru/testcase/alice-1405
    https://testpalm.yandex-team.ru/testcase/alice-1406
    https://testpalm.yandex-team.ru/testcase/alice-1407
    https://testpalm.yandex-team.ru/testcase/alice-1408
    https://testpalm.yandex-team.ru/testcase/alice-1409
    https://testpalm.yandex-team.ru/testcase/alice-1414
    https://testpalm.yandex-team.ru/testcase/alice-1434
    """

    owners = ('akormushkin', 'amullanurov', 'elkalinina',)

    channels = [
        'Все',
        'Первый',
        'Россия 1',
        'Матч!',
        'НТВ',
        'Пятый канал',
        'Культура',
        # 'Россия 24',
        'Карусель',
        'ОТР',
        'ТВ Центр',
        'РЕН ТВ',
        'Спас ТВ',
        'СТС',
        'Домашний',
        'ТВ-3',
        'Пятница',
        'Звезда',
        'МИР',
        'ТНТ',
        'МУЗ-ТВ',
        'Канал Disney',
    ]

    @pytest.mark.parametrize('command', [
        'что идет по телеку',
        'что идет по телевизору',
        'что по тв',
        'что по ящику',
    ])
    def test_alice_1404(self, alice, command):
        response = alice(command)

        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcast
        assert not response.directive
        assert response.text.startswith((
            'Вот что показывают сегодня.',
            'Вот что можно посмотреть сегодня.',
            'Вот что идёт сегодня.',
        ))
        assert response.div_card
        assert response.suggest('Вечером') or response.suggest('Завтра утром')
        assert response.suggest('Смотреть ТВ онлайн')

    def test_alice_1405_1406_1407_1408_1434(self, alice):
        response = alice('что по тв')

        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcast
        assert not response.directive

        channels = div_card.TvChannels(response.div_card)
        assert len(channels) == len(self.channels)
        for channel in channels:
            assert channel.name in self.channels
            if channel.name == 'Домашний':
                assert 4 <= len(channel.programs) <= 5
            else:
                assert len(channel.programs) == 5
            for program in channel.programs:
                assert program.name
            if channel.name == 'Звезда':
                assert channel.programs.first.has_stream()
            elif channel.name == 'Первый':
                assert not channel.programs.first.has_stream()

        assert 'ВСЕ ПЕРЕДАЧИ' in channels.footer.text
        assert channels.footer.action_url == 'https://tv.yandex.ru/?utm_content=tvmain&utm_source=alice&utm_medium=schedule'

    @pytest.mark.parametrize('command, channel_name', [
        ('что по первому каналу', 'Первый'),
        ('что идет на канале звезда', 'Звезда'),
    ])
    def test_alice_1409(self, alice, command, channel_name):
        response = alice(command)

        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcast
        assert not response.directive
        assert response.text in [
            f'Вот что идёт сегодня на канале {channel_name}.',
            f'Вот что показывают сегодня на канале {channel_name}.',
            f'Вот что можно посмотреть сегодня на канале {channel_name}.',
        ]

        channel = div_card.IndividualTvChannel(response.div_card)
        assert channel_name in channel.name
        assert len(channel.programs) >= 5
        for program in channel.programs:
            assert program.name
        assert 'ВСЕ ПЕРЕДАЧИ' in channel.footer.text

    def test_alice_1414(self, alice):
        response = alice('что по тв')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcast
        assert not response.directive

        time_suggest = response.suggest('Утром') or response.suggest('Днём') or \
            response.suggest('Вечером') or response.suggest('Завтра утром')
        assert time_suggest

        response = alice.click(time_suggest)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcastEllipsis
        assert not response.directive
        assert response.text.startswith((
            'Вот что показывают', 'Вот что можно посмотреть', 'Вот что идёт',
        )) and time_suggest.title.lower() in response.text
        assert response.div_card

        tv_online_suggest = response.suggest('Смотреть ТВ онлайн')
        assert tv_online_suggest

        response = alice.click(tv_online_suggest)
        assert response.scenario == scenario.Vins
        assert not response.directive
        assert response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.parametrize('surface', [surface.searchapp])
class TestWatchTv(object):

    owners = ('akormushkin', 'amullanurov',)

    def test_watch_tv_russia(self, alice):
        response = alice('смотреть тв в россии')
        assert response.scenario == scenario.Vins
        assert not response.directive
        assert response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]

    def test_watch_tv_netherlands(self, alice):
        response = alice('смотреть тв в нидерландах')
        if response.scenario == scenario.Vins:
            assert not response.directive
            assert response.text in [
                'Ой, простите, кажется, я не могу включить то, что вы просите в этом регионе.',
                'Я бы хотела помочь вам, но не могу - эфирные каналы недоступны в этом регионе.',
            ]
        else:
            assert response.scenario == scenario.Search

    @pytest.mark.region(region.Amsterdam)
    def test_watch_tv_region_netherlands(self, alice):
        response = alice('смотреть тв')
        assert response.scenario == scenario.Vins
        assert not response.directive
        assert response.text in [
            'Ой, простите, кажется, я не могу включить то, что вы просите в этом регионе.',
            'Я бы хотела помочь вам, но не могу - эфирные каналы недоступны в этом регионе.',
        ]


@pytest.mark.parametrize('surface', [surface.searchapp])
class TestPersonalChannel(object):

    owners = ('akormushkin',)

    @pytest.mark.parametrize('command', ['включи канал мой эфир', 'открой мой эфир'])
    def test_personal_channel(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert not response.directive
        assert response.text == 'Ваш персональный канал есть в Яндекс.Эфире, вот здесь.'
        assert response.button('Открыть')


@pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp, surface.yabro_win])
class TestUnavailableChannel(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1419
    """

    owners = ('akormushkin', 'doggywolf',)

    @pytest.mark.parametrize('command', ['включи 3 канал', 'включи канал россия'])
    def test_unavailable_existing_channel(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Search
        assert response.intent in [intent.Search, intent.Factoid] or response.text in [
            'Найдётся всё!',
            'Ищу ответ',
            'Ищу для вас ответ',
            'Открываю поиск',
            'Одну секунду...',
            'Сейчас найдём',
            'Ищу в Яндексе',
            'Сейчас найду',
            'Давайте поищем',
        ]

    def test_non_existing_channel(self, alice):
        channel = 'канал бебебе'
        response = alice(f'включи {channel}')
        assert response.scenario == scenario.Search or response.text in [
            'Такого канала, доступного для вашего региона, я не знаю, но есть другие. Смотрите прямо сейчас.',
            'Кажется, канала «бебебе» нет в вашем регионе. Вот что можно посмотреть.',
            'К сожалению, не смогла найти канал «бебебе» для вашего региона. Зато нашла другие, смотрите.',
        ]
        if response.scenario == scenario.Search:
            assert response.directive.name == directives.names.OpenUriDirective
            assert f'text={channel}' in unquote(response.directive.payload.uri)
        else:
            assert not response.directive


@pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp, surface.yabro_win])
class TestAvailableChannel(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1420
    """

    owners = ('akormushkin', 'doggywolf',)

    @pytest.mark.parametrize('command, channel_name', [
        ('включи канал РБК', 'РБК'),
        ('покажи канал звезда', 'Звезда'),
        ('включи канал мир 24', 'МИР 24'),
    ])
    def test_unavailable_existing_channel(self, alice, command, channel_name):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream
        assert not response.directive
        assert response.text in [
            f'Вот телеканал {channel_name}.',
            f'Нет проблем. Вот телеканал {channel_name}.',
        ]


@pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp])
class TestCommonTvQueries(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1421
    """

    owners = ('akormushkin', 'doggywolf',)

    @pytest.mark.parametrize('command', ['Смотреть ТВ', 'Включи телек', 'Включи прямой эфир', 'Покажи каналы онлайн'])
    def test_common_watch_tv(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream
        assert not response.directive
        assert response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]


@pytest.mark.parametrize('surface', [surface.launcher, surface.searchapp])
class TestPalmTvSuggests(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1423
    """

    owners = ('akormushkin', 'doggywolf',)

    def test_teleprogramm_suggest(self, alice):
        response = alice('Включи ТВ')
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream
        assert not response.directive
        assert response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]

        channel_suggest = False
        for suggest in response.suggests:
            if suggest.title.startswith('Канал'):
                channel_suggest = True
                break
        assert channel_suggest

        broadcast_suggest = response.suggest('Телепрограмма')
        assert broadcast_suggest
        if surface.is_searchapp(alice):
            response = alice.click(broadcast_suggest)
        else:
            response = alice('Телепрограмма')
        assert not response.directive
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvBroadcast
        assert response.text.startswith((
            'Вот что показывают', 'Вот что можно посмотреть', 'Вот что идёт',
        ))
        assert response.div_card

    @pytest.mark.xfail(reason='ALICE-10187')
    def test_channel_suggest(self, alice):
        response = alice('Включи телевизор')

        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream or response.text in [
            'Смотрите прямо сейчас.',
            'Вот что можно посмотреть. Выбирайте.',
        ]
        assert not response.directive

        broadcast_suggest = response.suggest('Телепрограмма')
        assert broadcast_suggest
        for suggest in response.suggests:
            if suggest.title.startswith('Канал'):
                channel_suggest = suggest
                break
        assert channel_suggest
        if surface.is_searchapp(alice):
            response = alice.click(channel_suggest)
        else:
            response = alice(channel_suggest.title)
        assert response.scenario == scenario.Vins
        assert response.intent == intent.TvStream
        assert not response.directive
        assert 'Вот телеканал' in response.text and channel_suggest.title in response.text
