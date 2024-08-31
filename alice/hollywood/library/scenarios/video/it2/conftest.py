import json
import pytest

from alice.hollywood.library.python.testing.it2 import surface, auth
from alice.hollywood.library.python.testing.it2.stubber import create_stubber_fixture, StubberEndpoint

from google.protobuf.json_format import MessageToDict
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse


def videobass_request_content_hasher(headers, content):
    message = TScenarioRunRequest()
    message.ParseFromString(content)

    result = json.dumps(MessageToDict(message), sort_keys=True)
    return result.encode('utf-8')


vh_stubber = create_stubber_fixture(
    'frontend.vh.yandex.ru',
    443,
    [
        StubberEndpoint('/player/{vh_uuid}.json', ['GET']),
        StubberEndpoint('/v23/carousel_videohub.json', ['GET']),
        StubberEndpoint('/v23/feed.json', ['GET']),
        StubberEndpoint('/v23/series_seasons.json', ['GET']),
        StubberEndpoint('/v23/series_episodes.json', ['GET']),
    ],
    scheme="https",
    stubs_subdir='player_stubs',
    hash_pattern=True
)
videosearch_stubber = create_stubber_fixture(
    host=('sas', 'hamster-app-host-sas-video-yp'),
    port=81,
    stubs_subdir='videosearch_stubs',
    scheme='grpc'
)
div_render_graph_stubber = create_stubber_fixture(
    ('sas', 'div-renderer-prestable.div-renderer'),
    10000,
    [
        StubberEndpoint('/render', ['POST']),
    ],
    stubs_subdir='div_render_back',
    type_to_proto={
        'mm_scenario_response': TScenarioRunResponse,
        'render_data': TDivRenderData,
        'render_result': TRenderResponse,
    },
    pseudo_grpc=True,
    header_filter_regexps=['content-length'],
)


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['video']


@pytest.fixture(scope='function')
def srcrwr_params(vh_stubber, videosearch_stubber, div_render_graph_stubber):
    return {
        'APP_HOST__VIDEO': f'localhost:{videosearch_stubber.port}',
        'HOLLYWOOD_VIDEO_PLAYER_PROXY': f'http://localhost:{vh_stubber.port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
    }


def load_json_device_state(path):
    with open(path, 'r', encoding='utf-8') as file:
        return json.loads(file.read())


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='Video', handle='video')
@pytest.mark.parametrize('surface', [surface.smart_tv])
class VideoScenarioTestingPreset:
    pass


@pytest.mark.oauth(auth.YandexPlus)
@pytest.mark.scenario(name='Video', handle='video')
# @pytest.mark.parametrize('surface', [surface.module_2]) # TODO dandex@
@pytest.mark.parametrize('surface', [surface.smart_tv])
@pytest.mark.experiments('video_use_pure_hw')
# user_agent needed in video hosting request to correctly get licenses on playing
@pytest.mark.additional_options(bass_options={
    "user_agent": "com.yandex.io.sdk/2.96.20.5292 (Yandex YandexModule2-00001; Android 9)"
})
class YaModuleTestingPreset:
    pass
