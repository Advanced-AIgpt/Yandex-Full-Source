import alice.tests.library.auth as auth
import alice.tests.library.directives as directives
import alice.tests.library.intent as intent
import alice.tests.library.scenario as scenario
import alice.tests.library.surface as surface
import pytest
from library.python import resource


@pytest.mark.parametrize('surface', [surface.station])
class _TestMordoviaVideoSelection(object):
    device_state = resource.find('mordovia_video_selection_device_state.json')


class TestMordoviaVideoSelectionScenario(_TestMordoviaVideoSelection):

    owners = ('akormushkin', )

    @pytest.mark.voice
    @pytest.mark.oauth(auth.YandexPlus)
    def test_play_video_strm(self, alice):
        response = alice('включи номер восемь')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'strm'

    @pytest.mark.oauth(auth.YandexPlus)
    def test_play_video_kinopoisk(self, alice):
        response = alice('открой гравити фолз')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective
        assert response.directive.payload.item.provider_name == 'kinopoisk'

    @pytest.mark.experiments('video_disable_webview_video_entity')
    @pytest.mark.parametrize('command', ['алиса, покажи описание три', 'описание сериала гравити фолз'])
    def test_show_film_description(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.ShowVideoDescriptionDirective

    @pytest.mark.parametrize('command', ['алиса, покажи описание три'])
    def test_show_webview_film_description(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.MordoviaShowDirective
        assert '/video/quasar/videoEntity/mainPage/?' in response.directive.payload.url


class TestMordoviaVideoSelectionExceptions(_TestMordoviaVideoSelection):

    owners = ('akormushkin',)

    @pytest.mark.parametrize('command', ['покажи кино в 4 к', '4к фильмы', 'сериалы в 4 k'])
    def test_4k_content(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Video

    @pytest.mark.voice
    @pytest.mark.parametrize('command', ['включи 1 канал', '2 канал', 'покажи 3 канал'])
    def test_tv_channels(self, alice, command):
        response = alice(command)
        assert response.scenario == scenario.Vins
        assert response.intent in {intent.TvStream, intent.TvBroadcast}

    @pytest.mark.oauth(auth.YandexPlus)
    @pytest.mark.parametrize('command', ['включи 1 плюс 1', 'запусти один дома'])
    def test_films_with_number_1_in_title(self, alice, command):
        first_item_id = alice.device_state.Video.ViewState['sections'][0]['items'][0]['metaforback']['uuid']
        response = alice(command)

        if response.directive.name in {directives.names.VideoPlayDirective, directives.names.ShowVideoDescriptionDirective}:
            # play or native description screen
            assert response.directive.payload.item.provider_item_id != first_item_id
        elif response.directive.name in {directives.names.MordoviaShowDirective, directives.names.MordoviaCommandDirective}:
            # webview description screen
            current_screen = alice.device_state.Video.ViewState['currentScreen']
            if current_screen.startswith('videoEntity'):
                current_item = alice.device_state.Video.ViewState['sections'][0].get_or_create_struct('current_item')
                assert current_item['provider_item_id'] != first_item_id

        # any other answer is OK in terms of this test case


@pytest.mark.voice
@pytest.mark.oauth(auth.YandexPlus)
class TestMordoviaPlayerFeatures(_TestMordoviaVideoSelection):

    owners = ('vitvlkv', )

    def test_play_cartoon_on_video_player_screen(self, alice):
        response = alice('включи номер восемь')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.VideoPlayDirective

        response = alice('стоп')
        assert response.scenario == scenario.Commands
        assert response.directive.name == directives.names.PlayerPauseDirective

        response = alice('включи мультик')
        assert response.scenario == scenario.MordoviaVideoSelection
        assert response.directive.name == directives.names.MordoviaShowDirective
