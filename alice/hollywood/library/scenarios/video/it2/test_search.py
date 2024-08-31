import base64

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice
from alice.protos.data.search_result.tv_search_result_pb2 import TTvSearchResultData

from conftest import YaModuleTestingPreset


default_commands = [
    pytest.param('открой', id='open'),
    pytest.param('включи', id='play'),
    pytest.param('найди', id='find'),
]


class TestOtt(YaModuleTestingPreset):
    def test_play_video(self, alice):
        r = alice(voice("Включи Моана"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'

        return str(r)

    def test_open_content_details(self, alice):
        r = alice(voice("Моана"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'

        return str(r)

    def test_paid(self, alice):
        r = alice(voice("Включи бойцовский клуб"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenDetailsScreenDirective'

        return str(r)


class TestSearch(YaModuleTestingPreset):
    def test_search_trailer_in_baseinfo(self, alice):
        r = alice(voice('фильм экипаж'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_search_cartoons(self, alice, command):
        r = alice(voice(f'{command} мультики'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_search_new(self, alice, command):
        r = alice(voice(f'{command} новинки'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_films(self, alice, command):
        r = alice(voice(f'{command} фильмы'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_porn(self, alice, command):
        r = alice(voice(f'{command} порно'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_search_collection(self, alice, command):
        r = alice(voice(f'{command} гарри поттера'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.parametrize('command', default_commands)
    def test_search_cats(self, alice, command):
        r = alice(voice(f'{command} видео с котиками'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'

        return str(r)

    @pytest.mark.device_state(device_config={'content_settings': 'safe'})
    def test_adult_with_safe_mode(self, alice):
        r = alice(voice('найди порно'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'
        speech = r.run_response.ResponseBody.Layout.OutputSpeech
        assert speech
        assert ('детски' in speech) and ('режим' in speech)

        return str(r)

    def test_complex_request(self, alice):
        r = alice(voice('включи самые новые топовые фильмы прикольные очень даже - США Великобритания 2020 год приключенческие из кинопоиска бесплатно'))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'TvOpenSearchScreenDirective'
        assert (directives[0].TvOpenSearchScreenDirective.SearchQuery == "топовые прикольные новые бесплатно фильмы очень даже - сша великобритания 2020 год приключенческие")

        return str(r)

    def test_empty_search_text(self, alice):
        r = alice(voice('раз 2 3 4 5 вышел зайчик погулять'))

        assert not r.run_response.ResponseBody.Layout.Directives
        return str(r)


@pytest.mark.oauth(auth.Amediateka)
class TestSerials(YaModuleTestingPreset):
    def test_simple(self, alice):
        r = alice(voice("Включи игра престолов"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Name == 'Игра престолов - Сезон 1 - Серия 1 - Зима близко'

        return str(r)

    @pytest.mark.xfail(reason="TODO: Улучшить!")
    def test_simple_with_action(self, alice):
        r = alice(voice("Включи игру престолов"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Name == 'Игра престолов - Сезон 1 - Серия 1 - Зима близко'

        return str(r)

    def test_with_season(self, alice):
        r = alice(voice("Включи игра престолов 4 сезон"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Name == 'Игра престолов - Сезон 4 - Серия 1 - Два меча'

        return str(r)

    @pytest.mark.parametrize('command', [pytest.param('Включи ', id='play'), pytest.param('', id='no_command')])
    def test_with_episode(self, alice, command):
        r = alice(voice(f"{command}игра престолов 3 серия"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Name == 'Игра престолов - Сезон 1 - Серия 3 - Лорд Сноу'

        return str(r)

    def test_with_season_and_episode(self, alice):
        r = alice(voice("Включи игра престолов 7 сезон 5 серия"))

        assert r.run_response.ResponseBody.Layout.Directives
        directives = r.run_response.ResponseBody.Layout.Directives
        assert len(directives) == 1
        assert directives[0].WhichOneof('Directive') == 'VideoPlayDirective'
        assert directives[0].VideoPlayDirective.Name == 'Игра престолов - Сезон 7 - Серия 5 - Восточный дозор'

        return str(r)


@pytest.mark.scenario(name='Video', handle='video')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class TestGRPCSearch:
    def test_simple(self, alice):
        payload = {
            'typed_semantic_frame': {'get_tv_search_result': {'search_text': {'string_value': 'Мультики'}}},
            'analytics': {'origin': 'Scenario', 'purpose': 'get_tv_search_result'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.run_response.ResponseBody.Layout.Directives
        assert r.run_response.ResponseBody.Layout.Directives[0].WhichOneof('Directive') == 'CallbackDirective'

        searchResultData = TTvSearchResultData()
        searchResultData.ParseFromString(
            base64.b64decode(
                r.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']
            )
        )
        return str(searchResultData)

    def test_search_by_entref(self, alice):
        payload = {
            'typed_semantic_frame': {
                'get_tv_search_result': {'search_entref': {'string_value': '0oCgpydXcxOTQzODMxGAJraOhY'}}
            },
            'analytics': {'origin': 'Scenario', 'purpose': 'get_tv_search_result'},
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert r.run_response.ResponseBody.Layout.Directives
        assert r.run_response.ResponseBody.Layout.Directives[0].WhichOneof('Directive') == 'CallbackDirective'

        searchResultData = TTvSearchResultData()
        searchResultData.ParseFromString(
            base64.b64decode(
                r.run_response.ResponseBody.Layout.Directives[0].CallbackDirective.Payload['grpc_response']
            )
        )

        firstItem = searchResultData.Galleries[0].BasicCarousel.Items[0]

        assert firstItem.VideoItem.Entref == "0oCgpydXcxOTQzODMxGAJraOhY"
        assert firstItem.VideoItem.Title == "Игра престолов"

        return str(searchResultData)
