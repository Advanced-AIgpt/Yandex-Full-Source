import logging
import pytest

from alice.hollywood.library.python.testing.it2.stubber import StubberEndpoint, create_stubber_fixture
from alice.kronstadt.fixture import Kronstadt
from alice.megamind.protos.scenarios.response_pb2 import TScenarioRunResponse
from alice.protos.api.renderer.api_pb2 import TDivRenderData, TRenderResponse


logger = logging.getLogger(__name__)


@pytest.fixture(scope='module')
def kronstadt_grpc_port(port_manager):
    return port_manager.get_port_range(None, 1)


@pytest.fixture(scope='module')
def kronstadt_http_port(port_manager):
    return port_manager.get_port_range(None, 1)


class DummyBassServer:
    def wait_port(self):
        pass


# quick fix for auto-default enabling bass_server ALICEINFRA-837
@pytest.fixture(scope='module')
def bass_server():
    return DummyBassServer()


@pytest.fixture(scope='module')
def scenario_runtime(kronstadt_http_port, kronstadt_grpc_port, enabled_scenarios):
    logger.info(f'enabled_scenarios={enabled_scenarios}')
    logger.info(f'http_port={kronstadt_http_port}')
    logger.info(f'grpc_port={kronstadt_grpc_port}')

    with Kronstadt(kronstadt_http_port, kronstadt_grpc_port, scenarios=enabled_scenarios) as service:
        yield service


@pytest.fixture(scope='function')
def generator_params():
    return ['timeout=100000']

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

render_result_response_merger_graph_stubber = create_stubber_fixture(
    host=('sas', 'hollywood-trunk-base'),
    port=81,
    scheme='grpc',
    stubs_subdir='hollywood_response_merger',
    type_to_proto={
        'mm_scenario_response': TScenarioRunResponse,
        'render_result': TRenderResponse,
    },
    source_name_filter={'RENDER_DIV2', 'SCENARIO_RESPONSE'}
)
