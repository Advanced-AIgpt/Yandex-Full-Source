import logging

import pytest
from alice.hollywood.library.python.testing.it2 import auth, surface
from alice.hollywood.library.python.testing.it2.input import server_action, voice, Scenario
from alice.hollywood.library.python.testing.it2.scenario_responses import proto_to_text
from alice.hollywood.library.python.testing.it2.stubber import HttpResponseStub
from alice.protos.api.renderer.api_pb2 import TDivRenderData
from alice.protos.data.scenario.music.content_id_pb2 import TContentId
from alice.protos.data.scenario.music.content_info_pb2 import TContentInfo


logger = logging.getLogger(__name__)

EXPERIMENTS = [
    'hw_music_show_view',
    'hw_music_thin_client',
    'hw_music_thin_client_playlist',
]

FM_RADIO_EXPERIMENTS = [
    'hw_music_thin_client_fm_radio',
    'radio_play_in_search',
    'radio_play_in_quasar',
]

GENERATIVE_EXPERIMENTS = [
    'hw_music_thin_client_generative',
]


def build_payload(object_type, object_id, start_from_track_id=None):
    payload = {
        'typed_semantic_frame': {
            'music_play_semantic_frame': {
                'object_id': {
                    'string_value': object_id,
                },
                'object_type': {
                    'enum_value': object_type,
                },
                'disable_nlg': {
                    'bool_value': False,
                },
            },
        },
        'analytics': {
            'origin': 'Scenario',
            'purpose': 'play_music',
        },
    }

    if start_from_track_id is not None:
        payload['typed_semantic_frame']['music_play_semantic_frame']['start_from_track_id'] = {
            'string_value': start_from_track_id,
        }
        pass

    return payload


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='HollywoodMusic', handle='music')
@pytest.mark.experiments(*EXPERIMENTS)
@pytest.mark.parametrize('surface', [surface.smart_display])
@pytest.mark.freeze_stubs(music_back_stubber={
    '/internal-api/users/{user_id}/likes/tracks': [
        HttpResponseStub(200, 'freeze_stubs/likes_tracks.json'),
    ],
    '/internal-api/users/{user_id}/dislikes/tracks': [
        HttpResponseStub(200, 'freeze_stubs/dislikes_tracks.json'),
    ],
})
class _TestsDivRenderBase:
    '''
    У тестового юзера очень большое количество лайков и дизлайков (порядка ~10 000),
    это мешает разработке (очень большие диффы),
    поэтому используем поддельные стабы с адекватным количеством треков.
    '''

    DIV_RENDER_NODE_NAME = 'DIV_RENDER'
    RENDER_DATA_MESSAGE_TYPE = 'render_data'

    def _get_render_data(self, response):
        div_render_req = response.sources_dump.get_grpc_request(self.DIV_RENDER_NODE_NAME)
        assert div_render_req

        render_data = div_render_req.get_protobuf_answer(TDivRenderData, self.RENDER_DATA_MESSAGE_TYPE)
        assert render_data

        return render_data


class TestsDivRender(_TestsDivRenderBase):

    NEIGHBORING_TRACKS_HTTP_NODE_NAME = 'MUSIC_SCENARIO_THIN_CONTENT_NEIGHBORING_TRACKS_PROXY'

    @pytest.mark.experiments('hw_music_show_view_neighboring_tracks_count=5')
    @pytest.mark.parametrize('command', [
        pytest.param('включи рамштайн', id='artist'),
        pytest.param('включи альбом mutter', id='album'),
        pytest.param('включи плейлист 5000+', id='playlist'),
    ])
    def test_simple(self, alice, command):
        '''
        В ноду "DIV_RENDER" посылается протобуф с указанием, какую картинку отрисовать.
        Нужно канонизировать этот протобуф и проверить что он правильно составлен.
        '''
        r = alice(voice(command))
        layout = r.continue_response.ResponseBody.Layout

        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('AudioPlayDirective')

        # smoke проверка что был запрос в ноду за "соседними" треками
        assert r.sources_dump.get_http_request(self.NEIGHBORING_TRACKS_HTTP_NODE_NAME)

        result = []

        # запрашивается 20 треков, но в девайсе показывается 6 треков
        # "слева" 0 треков, "справа" 5 треков
        render_data = self._get_render_data(r)
        queue_items = render_data.ScenarioData.MusicPlayerData.QueueItems
        assert len(queue_items) == 6
        titles_1 = [item.Title for item in queue_items]
        result.append(render_data)

        # когда текущий трек закончится, в очереди останется 18 треков
        # в девайсе показывается 7 треков: "слева" 1 трек, "справа" 5 треков
        r = alice(voice('следующий трек'))
        render_data = self._get_render_data(r)
        queue_items = render_data.ScenarioData.MusicPlayerData.QueueItems
        assert len(queue_items) == 7
        titles_2 = [item.Title for item in queue_items]
        result.append(render_data)

        # снова: когда текущий трек закончится, в очереди останется 17 треков
        # в девайсе показывается 8 треков: "слева" 2 трека, "справа" 5 треков
        r = alice(voice('следующий трек'))
        render_data = self._get_render_data(r)
        queue_items = render_data.ScenarioData.MusicPlayerData.QueueItems
        assert len(queue_items) == 8
        titles_3 = [item.Title for item in queue_items]
        result.append(render_data)

        # проверка что сначала показали 6 треков, потом показали те же треки
        # плюс 1 новый трек справа, и так далее
        assert len(titles_1) == 6
        assert titles_1 == titles_2[:-1]
        assert titles_2 == titles_3[:-1]

        # канонизируем все render_data
        canon = ''
        for idx, render_data in enumerate(result):
            canon += f'### Render data #{idx}\n'
            canon += proto_to_text(render_data)
            canon += '\n'
        return canon

    @pytest.mark.experiments('hw_music_show_view_neighboring_tracks_count=5')
    @pytest.mark.parametrize('command', [
        pytest.param('включи du hast', id='track'),
        pytest.param('включи рок', id='radio'),
    ])
    def test_no_request(self, alice, command):
        '''
        Если запрос не про альбом/плейлист/исполнителя, то не нужно отправлять
        запрос в NEIGHBORING_TRACKS. Потому что лишний запрос затормозит сценарий.
        '''
        r = alice(voice(command))
        layout = r.continue_response.ResponseBody.Layout

        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('AudioPlayDirective')

        render_data = self._get_render_data(r)
        assert len(render_data.ScenarioData.MusicPlayerData.QueueItems) == 0

        assert not r.sources_dump.get_http_request(self.NEIGHBORING_TRACKS_HTTP_NODE_NAME)

    @pytest.mark.experiments('hw_music_show_view_neighboring_tracks_count=3')
    def test_semantic_frame(self, alice):
        '''
        Тестирование запроса через semantic frame. Через semantic frame можно "попасть"
        в самую середину плейлиста. Также тест проверяет, что "лишние" треки из
        загруженной страницы будут выкинуты.
        Списки треков канонизируются.
        '''
        first_track_id = '12227710'
        payload = build_payload(
            object_type='Playlist',
            object_id='yndx.epislon:1044',  # плейлист с 5000+ треками
            start_from_track_id=first_track_id,  # трек ближе к середине плейлиста
        )

        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        layout = r.continue_response.ResponseBody.Layout

        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('AudioPlayDirective')

        # smoke проверка что был запрос в ноду за "соседними" треками
        assert r.sources_dump.get_http_request(self.NEIGHBORING_TRACKS_HTTP_NODE_NAME)

        result = []

        # показывается 7 треков: 3 "слева", 1 "посередине", 3 "справа"
        render_data = self._get_render_data(r)
        queue_items = render_data.ScenarioData.MusicPlayerData.QueueItems
        assert len(queue_items) == 7
        titles = [item.Title for item in queue_items]
        result.append(render_data)

        assert queue_items[3].Id == first_track_id  # запрошенный трек находится "посередине"

        # трижды просим "следующий трек", в очереди должно быть 7 треков,
        # очередь сдвигается на 1 трек "вперед"
        for i in range(3):
            r = alice(voice('следующий трек'))
            render_data = self._get_render_data(r)
            queue_items = render_data.ScenarioData.MusicPlayerData.QueueItems
            assert len(queue_items) == 7
            result.append(render_data)

            new_titles = [item.Title for item in queue_items]
            assert titles[1:] == new_titles[:-1]
            titles = new_titles

        # канонизируем все render_data
        canon = ''
        for idx, render_data in enumerate(result):
            canon += f'### Render data #{idx}\n'
            canon += proto_to_text(render_data)
            canon += '\n'
        return canon

    @pytest.mark.experiments('hw_music_show_view_neighboring_tracks_count=0')
    @pytest.mark.parametrize('payload, content_info', [
        pytest.param(build_payload('Track', '22771'), TContentInfo(Title='Du Hast'), id='track'),  # track "Du Hast"
        pytest.param(build_payload('Album', '7166032'), TContentInfo(Title='Sehnsucht'), id='album'),  # album "Sehnsucht"
        pytest.param(build_payload('Artist', '13002'), TContentInfo(Name='Rammstein'), id='artist'),  # artist "Rammstein"
        pytest.param(build_payload('Playlist', 'yndx.epislon:1044'), TContentInfo(Title='5000+'), id='playlist'),
    ])
    def test_content_info_semantic_frame(self, alice, payload, content_info):
        '''
        В запросе к div render должен быть заполнен content info,
        даже если запрос через semantic frame
        '''
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))
        assert content_info == self._get_render_data(r).ScenarioData.MusicPlayerData.ContentInfo

    @pytest.mark.experiments(*FM_RADIO_EXPERIMENTS)
    @pytest.mark.experiments('hw_music_show_view_neighboring_tracks_count=0')
    def test_switch_from_fm_radio(self, alice):
        '''
        Раньше был баг, что при смене проигрывания с FMRadio неправильно ставился ContentId Type
        '''
        r = alice(voice('включи радио шансон'))

        payload = build_payload('Playlist', 'yndx.epislon:1044')
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        data = self._get_render_data(r).ScenarioData.MusicPlayerData
        assert data.ContentInfo == TContentInfo(Title='5000+')
        assert data.ContentId == TContentId(Type=TContentId.EContentType.Playlist, Id='178190693:1044')


@pytest.mark.experiments(*FM_RADIO_EXPERIMENTS)
class TestsFmRadioDivRender(_TestsDivRenderBase):

    def test_general(self, alice):
        r = alice(voice('включи радио'))

        layout = r.continue_response.ResponseBody.Layout
        assert len(layout.Directives) == 3
        assert layout.Directives[0].HasField('ShowViewDirective')
        assert layout.Directives[1].HasField('TtsPlayPlaceholderDirective')
        assert layout.Directives[2].HasField('AudioPlayDirective')

        render_data = self._get_render_data(r)
        assert len(render_data.ScenarioData.MusicPlayerData.QueueItems) > 0  # check that we send items
        return proto_to_text(render_data)

    def test_after_pause(self, alice):
        '''
        Раньше при запросе "продолжи" с паузы мы не отправляли список item-ов
        и очередь становилась пустой. Это было из-за того, что раньше запрос
        за списком радиостанций отправляли только при смене радиостанции.
        '''
        r = alice(voice('включи радио'))
        r = alice(voice('хватит', scenario=Scenario('Commands', 'fast_command')))

        payload = {
            'typed_semantic_frame': {
                'player_continue_semantic_frame': {
                    'disable_nlg': {
                        'bool_value': True,
                    },
                },
            },
            'analytics': {
                'origin': 'Scenario',
                'purpose': 'play_music',
            },
        }
        r = alice(server_action(name='@@mm_semantic_frame', payload=payload))

        assert not r.continue_response.ResponseBody.Layout.OutputSpeech  # nlg is disabled

        render_data = self._get_render_data(r)
        assert len(render_data.ScenarioData.MusicPlayerData.QueueItems) > 0  # check that we send items
        return proto_to_text(render_data)


@pytest.mark.experiments(*GENERATIVE_EXPERIMENTS)
class TestsGenerativeDivRender(_TestsDivRenderBase):

    def test_general(self, alice):
        r = alice(voice('включи генеративную музыку'))
        return proto_to_text(self._get_render_data(r))
