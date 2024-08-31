import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmFindTvChannel(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1014
    """

    owners = ('akormushkin',)

    @pytest.mark.parametrize('command', [
        'Найди канал Звезда',
        'Включи телеканал Дождь',
        'Прямой эфир RUTV',
        'Включи РБК',
    ])
    def test_alice_1014(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.directive
        assert response.directive.name in {directives.names.VideoPlayDirective, directives.names.ShowTvGalleryDirective}

        if response.directive.name == directives.names.VideoPlayDirective:
            assert ''.join(response.directive.payload.item.name.lower().split()) == command.split()[-1].lower()

        if response.directive.name == directives.names.ShowTvGalleryDirective:
            assert response.text.startswith((
                'На этом канале сейчас нет вещания.',
                'Такого канала нет или он недоступен для вещания в вашем регионе.',
            ))
            assert response.text.endswith((
                'Давайте посмотрим что-нибудь ещё.',
                'Но есть много других каналов. Смотрите.',
            ))


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmVideoTv(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1034
    https://testpalm.yandex-team.ru/testcase/alice-1915
    https://testpalm.yandex-team.ru/testcase/alice-2066
    """

    owners = ('akormushkin', )

    valid_answers = [
        'Вот доступные каналы.',
        'Вот каналы для вашего региона.',
        'Давайте что-нибудь посмотрим.',
    ]

    @pytest.mark.parametrize('command', [
        'Что идёт сейчас по тв',
        'Хочу посмотреть тв',
        'А что сейчас идет по телеку',
        pytest.param(
            'Что сегодня по телеку',
            marks=pytest.mark.xfail(reason='выигрывает Vins вместо ShowTvChannelsGallery: https://st.yandex-team.ru/VIDEOFUNC-887')
        ),
        pytest.param(
            'Что сегодня идет по телеку',
            marks=pytest.mark.xfail(reason='выигрывает Vins вместо ShowTvChannelsGallery: https://st.yandex-team.ru/VIDEOFUNC-887')
        ),
        pytest.param(
            'Что по телеку интересного вообще',
            marks=pytest.mark.xfail(reason='выигрывает Vins вместо ShowTvChannelsGallery: https://st.yandex-team.ru/VIDEOFUNC-887')
        ),
        'А что по телеку',
        'ТВ онлайн',
        'Прямой эфир',
        'Смотреть прямой эфир',
        'Онлайн телевидение',
        'Список каналов',
    ])
    def test_alice_1034(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert response.text in self.valid_answers

    @pytest.mark.oauth(auth.Yandex)
    def test_alice_1915(self, alice):
        response = alice('что по тв')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert response.text in self.valid_answers

    def test_alice_2066(self, alice):
        alice('домой')
        response = alice('Что идёт сейчас по тв')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert response.text in self.valid_answers


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestSelectChannelFromGallery(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1285
    """

    owners = ('akormushkin',)

    def test_alice_1285_native(self, alice):
        response = alice('Покажи список каналов')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        all_items = alice.device_state.Video.ScreenState.RawItems
        response = alice('Включи номер три')
        assert response.scenario == scenario.Video
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.type == 'tv_stream'
        assert response.directive.payload.item.name == all_items[2].Name

    @pytest.mark.experiments('tv_channels_webview')
    def test_alice_1285_webview(self, alice):
        response = alice('Покажи список каналов')
        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/channels' in response.directive.payload.url

        all_items = alice.device_state.Video.ViewState['sections'][0]['items']
        response = alice('Включи номер три')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.type == 'tv_stream'
        assert response.directive.payload.item.name.startswith(all_items[2]['name'])


@pytest.mark.voice
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmVideoTvNavigation(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1040
    """

    owners = ('akormushkin', 'nkodosov',)

    def test_alice_1040(self, alice):
        response = alice('что по тв')

        assert response.scenario == scenario.ShowTvChannelsGallery
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective
        assert response.text in [
            'Вот доступные каналы.',
            'Вот каналы для вашего региона.',
            'Давайте что-нибудь посмотрим.',
        ]

        all_items = alice.device_state.Video.ScreenState.RawItems

        response = alice('номер 2')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['name'] == all_items[1].Name

        response = alice('дальше')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['name'] == all_items[2].Name

        response = alice('Включи предыдущий')
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item['name'] == all_items[1].Name


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmTvPauseAndContinue(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1041
    """

    owners = ('akormushkin',)

    def test_alice_1041(self, alice):
        response = alice('включи канал РБК')
        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name.lower() == 'рбк'

        alice.skip(20)  # "смотрим эфир" некоторое время
        alice('поставь на паузу')
        alice.skip(30)  # ждём

        response = alice('продолжи')

        assert response.directive
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.name.lower() == 'рбк'
        assert response.directive.payload.start_at == 0


@pytest.mark.voice
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmTvUnsupportedChannel(object):
    """
    https://testpalm2.yandex-team.ru/testcase/alice-1277
    """

    def test_alice_1277(self, alice):
        response = alice('включи НТВ')
        assert response.directive
        assert response.directive.name == directives.names.ShowTvGalleryDirective

        assert response.text.startswith('Такого канала нет или он недоступен для вещания в вашем регионе.')
        assert response.text.endswith((
            'Давайте посмотрим что-нибудь ещё.',
            'Но есть много других каналов. Смотрите.'
        ))


@pytest.mark.voice
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station])
class TestPalmTvProgram(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1300
    """

    owners = ('akormushkin', 'amullanurov', 'elkalinina',)

    def test_alice_1300(self, alice):
        response = alice('телепрограмма')
        assert not response.directive
        # Программа на ближайшее время на 5 каналов
        assert response.text.count('\n') == 4

        response = alice('что по программе вечером')
        assert not response.directive
        # Программа на вечернее время на 5 каналов
        assert response.text.count('\n') == 4

        response = alice('что завтра по телевизору')
        assert not response.directive
        # Программа на утро завтрашнего дня на 5 каналов
        assert response.text.count('\n') == 4


@pytest.mark.xfail(reason='https://st.yandex-team.ru/ALICE-1112')
@pytest.mark.voice
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [surface.station(is_tv_plugged_in=False)])
class TestPalmTvProgramWithoutScreen(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1350
    """

    owners = ('akormushkin', 'amullanurov', 'elkalinina',)

    def test_alice_1350(self, alice):
        response = alice('телепрограмма первого канала')
        assert not response.directive
        assert response.has_voice_response()
        assert response.output_speech_text.startswith((
            'Вот что можно посмотреть сегодня на канале Первый.',
            'Вот что показывают сегодня на канале Первый.',
            'Вот что идёт сегодня на канале Первый.',
        ))

        response = alice('телепрограмма на сегодня по каналу звезда')
        assert not response.directive
        assert response.has_voice_response()
        assert response.output_speech_text.startswith((
            'Вот что можно посмотреть сегодня на канале Звезда.',
            'Вот что показывают сегодня на канале Звезда.',
            'Вот что идёт сегодня на канале Звезда.',
        ))

        response = alice('телепрограмма канала табуретка')
        assert not response.directive
        assert response.has_voice_response()
        assert response.output_speech_text.startswith((
            'Кажется, канала «табуретка» нет в вашем регионе.',
            'К сожалению, не смогла найти канал «табуретка» для вашего региона.',
            'Такого канала, доступного для вашего региона, я не знаю, но есть другие.',
        ))


@pytest.mark.voice
@pytest.mark.oauth(auth.Yandex)
@pytest.mark.parametrize('surface', [
    surface.loudspeaker,
    surface.station(is_tv_plugged_in=False),
])
class TestPalmTvCallWithoutScreen(object):
    """
    https://testpalm.yandex-team.ru/testcase/alice-1329
    """

    owners = ('akormushkin', 'doggywolf',)

    @pytest.mark.parametrize('command', ['Включи телевизор', 'Включи ТВ'])
    def test_tv_queries_without_tv(self, alice, command):
        response = alice(command)

        assert (response.has_voice_response() and response.output_speech_text.startswith((
            'Не умею.',
            'На этом устройстве не получится.',
            'Здесь, к сожалению, не выйдет.',
            'Здесь, к сожалению, не выйдет.',
        ))) or response.intent in [intent.TvBroadcast, intent.Factoid] or response.directive.name == directives.names.MusicPlayDirective

        response = alice('Включи первый канал')
        assert (response.has_voice_response() and response.output_speech_text.startswith((
            'Я не могу включить этот канал. Но у меня есть его расписание.',
            'Боюсь, миссия невыполнима. Зато у меня есть расписание этого канала.',
        ))) or response.intent in [intent.TvBroadcast, intent.Factoid] or response.directive.name == directives.names.MusicPlayDirective
