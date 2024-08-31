import pytest
from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_localhost_bass_stubber_fixture, \
    create_stubber_fixture
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse


@pytest.fixture(scope='module')
def enabled_scenarios():
    return ['news']


def create_memento_stubber_fixture(tests_data_path):
    return create_stubber_fixture(
        tests_data_path,
        'paskills-common-testing.alice.yandex.net',
        80,
        [
            StubberEndpoint('/memento/get_objects', ['POST']),
            StubberEndpoint('/memento/update_objects', ['POST']),
        ],
        stubs_subdir='memento',
    )


bass_stubber = create_localhost_bass_stubber_fixture()

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


@pytest.fixture(scope="function")
def srcrwr_params(bass_stubber, div_render_graph_stubber):
    return {
        'NEWS_SCENARIO_PROXY': f'localhost:{bass_stubber.port}',
        'NEWS_SCENARIO_APPLY_PROXY': f'localhost:{bass_stubber.port}',
        'RENDER_DIV2': f'localhost:{div_render_graph_stubber.port}',
    }
