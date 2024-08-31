import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest

from music.thin_client import assert_response, assert_audio_play_directive


EXPERIMENTS_THIN_CLIENT_GENERATIVE = [
    'hw_music_thin_client_generative',
    'bg_fresh_granet',
]

DEFAULT_RESPONSE_RE = r'.*(настроит на нужный лад|вдохнов|немного драйва|Нейро музык|расслабл.).*'
FOCUS_RESPONSE_RE = r'.*(настроит на нужный лад|вдохнов).*'
ENERGY_RESPONSE_RE = r'.*(немного драйва|Нейромузык).*'
RELAX_RESPONSE_RE = r'.*расслабл.*'


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.experiments(*EXPERIMENTS_THIN_CLIENT_GENERATIVE)
@pytest.mark.parametrize('surface', [surface.station])
@pytest.mark.version(hollywood=176)
class TestGenerativeMusic(object):

    owners = ('amullanurov',)

    @pytest.mark.parametrize('command, text_re', [
        pytest.param('включи нейромузыку', DEFAULT_RESPONSE_RE,  id='default'),
        pytest.param('включи бодрящую нейронную музыку', ENERGY_RESPONSE_RE, id='energy'),
        pytest.param('включи нейронную музыку для сосредоточения', FOCUS_RESPONSE_RE, id='focus'),
        pytest.param('включи музыку для сна', RELAX_RESPONSE_RE, marks=pytest.mark.skip(reason='waiting for granet'), id='relax'),
    ])
    def test_play_generative_music(self, alice, command, text_re):
        response = alice(command)
        assert_response(response, text_re=text_re, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

    def test_play_generative_music_stop_continue(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        first_track_url = response.directive.payload.stream.url

        response = alice('стоп')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)
        response = alice('продолжи')
        assert not response.text
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.stream.url == first_track_url

    @pytest.mark.experiments('music_thin_client_generative_force_reload_on_stream_play')
    def test_play_generative_music_stop_continue_force_reload(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        first_track_url = response.directive.payload.stream.url

        response = alice('стоп')
        assert_response(response, text_is_none=True, scenario=scenario.Commands,
                        directive=directives.names.ClearQueueDirective)
        response = alice('продолжи')
        assert not response.text
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.MmStackEngineGetNextCallback

        response.next()
        assert not response.text
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.stream.url != first_track_url

    def test_play_generative_music_like(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('поставь лайк')
        assert any([sub_text in response.text for sub_text in [
            'лайк', 'вы оценили', 'вам такое по душе', 'Буду включать такое чаще.',
        ]]), f'response.text={response.text}'
        assert not response.directive

    def test_play_generative_music_dislike(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        first_track_url = response.directive.payload.stream.url

        response = alice('поставь дизлайк')
        assert response.text in [
            'Поняла, сейчас алгоритмы напишут что-то другое.',
            'Окей, алгоритмы Нейро музыки уже учли ваш дизлайк.',
            'О вкусах не спорят. Попрошу Нейро музыку подстроиться под ваш.',
        ]
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.MmStackEngineGetNextCallback

        response.next()
        assert not response.text
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.stream.url != first_track_url

    def test_play_generative_music_skip(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        first_track_url = response.directive.payload.stream.url

        response = alice('пропусти')
        assert response.text == 'Пропускаю.'
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.MmStackEngineGetNextCallback

        response.next()
        assert not response.text
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.stream.url != first_track_url

    def test_play_generative_music_next_prev_track_different_content_types(self, alice):
        #  history = Краш (играет)
        response = alice('включи песню краш')
        assert_response(response, text='Включаю: Клава Кока, NILETTO, песня \"Краш\".', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        #  history = Краш, Синий (играет)
        response = alice('включи нейронную музыку для сосредоточения')
        assert_response(response, text_re=FOCUS_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        #  history = Краш, Синий, Numb (играет)
        response = alice('включи песню numb')
        assert_response(response, text='Включаю: Linkin Park, песня \"Numb\".', scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        #  history = Краш, Синий (играет)
        response = alice('предыдущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, title='Синий')

        #  history = Краш (играет), queue = Синий
        response = alice('предыдущий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, title='Краш')

        #  history = Краш, Синий (играет)
        response = alice('следующий трек')
        assert_response(response, text_is_none=True, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, title='Вдохновение')
        first_track_url = response.directive.payload.stream.url

        response = alice('следующий трек')
        assert response.text == 'Пропускаю.'
        assert len(response.directives) == 1
        assert response.directives[0].name == directives.names.MmStackEngineGetNextCallback

        response.next()
        assert not response.text
        assert_audio_play_directive(response.directive)
        assert response.directive.payload.stream.url != first_track_url

    def test_play_generative_music_prev_track_with_thick_client(self, alice):
        #  No history, thick client
        response = alice('включи шум моря')
        assert response.scenario == scenario.Vins
        assert response.directive.name == directives.names.MusicPlayDirective
        assert response.text.lower() == 'включаю шум моря.'

        #  history = Синий (играет)
        response = alice('включи нейронную музыку для сосредоточения')
        assert_response(response, text_re=FOCUS_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive, title='Вдохновение')

        #  No history
        response = alice('предыдущий трек')
        assert response.scenario == scenario.HollywoodMusic
        assert response.text in [
            'Извините, я не запомнила, какой трек был предыдущим.',
            'Простите, я отвлеклась и не запомнила, что играло до этого.',
            'Простите, я совершенно забыла, что включала до этого.',
            'Извините, но я выходила во время предыдущего трека и не знаю, что играло.',
        ]

    def test_play_generative_music_what_is_playing(self, alice):
        response = alice('включи нейромузыку для сосредоточения')
        assert_response(response, text_re=FOCUS_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)

        response = alice('что сейчас играет')
        assert_response(response, text_re=r'.*нейромузыка на станции \"Вдохновение\".*', scenario=scenario.HollywoodMusic)
        assert not response.directive

    @pytest.mark.parametrize('command, text', [
        pytest.param('перемотай назад на 30 секунд', 'Простите, я не могу перемотать нейромузыку.', id='rewind_backward'),
        pytest.param('перемотай вперед на 30 секунд', 'Простите, я не могу перемотать нейромузыку.', id='rewind_forward'),
        pytest.param('перемешай музыку', 'Простите, я не могу перемешать нейромузыку.', id='shuffle'),
        pytest.param('поставь на повтор', 'Простите, не могу поставить на повтор нейромузыку.', id='repeat'),
        pytest.param('включи заново', 'Простите, я не могу повторить нейромузыку.', id='replay'),
    ])
    def test_play_generative_music_unsupported_commands(self, alice, command, text):
        response = alice('включи нейромузыку')
        assert_response(response, text_re=DEFAULT_RESPONSE_RE, scenario=scenario.HollywoodMusic)
        assert_audio_play_directive(response.directive)
        alice.skip(seconds=40)

        response = alice(command)
        assert response.text == text


@pytest.mark.supported_features('music_sdk_client')
@pytest.mark.experiments('internal_music_player', *EXPERIMENTS_THIN_CLIENT_GENERATIVE)
@pytest.mark.parametrize('surface', [surface.searchapp, surface.navi, surface.legatus, surface.dexp])
@pytest.mark.version(hollywood=177)
class TestsGenerativeUnsupportedSurfaces(object):

    owners = ('amullanurov',)

    def test_play_generative(self, alice):
        response = alice('включи нейромузыку')
        assert_response(response, text='Простите, не могу включить нейромузыку в этом приложении.', scenario=scenario.HollywoodMusic)
