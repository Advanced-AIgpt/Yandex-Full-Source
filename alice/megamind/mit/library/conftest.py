import logging
import pytest
import os
from typing import List, Dict, Set
import yatest.common as yc
from yatest.common.network import PortManager

from alice.megamind.mit.library.generator import MitGenerator
from alice.megamind.mit.library.graphs_util import ah_graphs_info, GraphsInfo, NodeWrapper
from alice.megamind.mit.library.requester import MegamindRequester
from alice.megamind.mit.library.runner import MitRunner
from alice.megamind.mit.library.stubber import ApphostStubberService, StubberRepository
from alice.megamind.mit.library.util import is_generator_mode, iterate_markers, EVENTLOG_FILE_NAME
from alice.tests.library.service import AppHost, Megamind


logger = logging.getLogger(__name__)


UNSTUBBING_SUBGRAPH_NODES: Set[str] = {
    'COMBINATORS_CONTINUE',
    'COMBINATORS_RUN',
    'SCENARIOS_APPLY_STAGE',
    'SCENARIOS_COMMIT_STAGE',
    'SCENARIOS_CONTINUE_STAGE',
    'SCENARIOS_RUN_STAGE',
    'BEGEMOT_STAGE',
}

MEGAMIND_BACKENDS: Set[str] = {
    'MEGAMIND',
    'MEGAMIND_STREAMING',
}


@pytest.fixture(scope='session')
def port_manager():
    with PortManager() as pm:
        yield pm


@pytest.fixture(scope='module')
def megamind(port_manager: PortManager):
    port = port_manager.get_port_range(None, Megamind.port_count())
    cmd_params: List[str] = [
        '--vins-package-abs-path', yc.source_path(
            'alice/vins/packages/vins_package.json'),
        '--local',
        '--formulas-path', os.path.join(yc.runtime.work_path(),
                                        'megamind_formulas'),
        '--geobase-path', os.path.join(yc.runtime.work_path(), 'geodata6.bin'),
        '--partial-pre-classification-model-path', os.path.join(
            yc.runtime.work_path(), 'partial_preclf_model.cbm')
    ]
    with Megamind(port, additional_cmd_params=cmd_params) as server:
        yield server


@pytest.fixture(scope='module')
def graphs_info() -> GraphsInfo:
    return ah_graphs_info(
        graphs_dir=yc.source_path('apphost/conf/verticals/ALICE'),
        backends_dir=yc.source_path('apphost/conf/backends'),
        root_graphs_names={'megamind', 'megamind_apply'},
        allowed_subgraph_nodes=UNSTUBBING_SUBGRAPH_NODES,
    )


@pytest.fixture(scope='module')
def megamind_nodes(graphs_info: GraphsInfo) -> Dict[str, NodeWrapper]:
    megamind_nodes: Dict[str, NodeWrapper] = dict()
    for megamind_backend in MEGAMIND_BACKENDS:
        megamind_nodes.update(graphs_info.nodes_with_backend(megamind_backend))
    return megamind_nodes


@pytest.fixture(scope='module')
def apphost(port_manager: PortManager):
    port = port_manager.get_port_range(None, AppHost.port_count())
    with AppHost(
        port=port,
        disable_all_remote_backends=(not is_generator_mode()),
        launch_timeout_seconds=3000
    ) as service:
        yield service


@pytest.fixture(scope='module')
def eventlog_path() -> str:
    return os.path.join(yc.runtime.work_path(), EVENTLOG_FILE_NAME)


@pytest.fixture(scope='module')
def module_path(request) -> str:
    module_path = os.path.abspath(request.module.__file__)
    start = module_path.rfind('alice/megamind')
    end = module_path.rfind('/')
    result = module_path[start:end]
    logger.debug(f'test_path={result}')
    return result


@pytest.fixture(scope='module')
def stubber_repository(eventlog_path: str, module_path: str):
    with StubberRepository(eventlog_path, module_path) as repo:
        yield repo


@pytest.fixture(scope='function')
def test_name(request):
    logger.debug(f'test_name: {request.node.name}')
    return request.node.name


@pytest.fixture(scope='function')
def apphost_stubber(
    port_manager: PortManager,
    stubber_repository: StubberRepository,
    test_name: str,
    graphs_info: GraphsInfo,
    megamind_nodes: Dict[str, NodeWrapper],
    megamind: Megamind,
):
    port = port_manager.get_port_range(
        None, ApphostStubberService.port_count())
    with ApphostStubberService(
        port,
        stubber_repository,
        graphs_info,
        megamind_nodes,
        megamind_port=megamind.grpc_port,
        test_name=test_name,
        unstubbing_subgraph_nodes=UNSTUBBING_SUBGRAPH_NODES,
    ) as service:
        yield service


@pytest.fixture(scope='function')
def experiments(request) -> Dict[str, str]:
    experiments = dict()
    for marker in iterate_markers(request, 'experiments'):
        experiments.update({it: '1' for it in marker.args})
        experiments.update(marker.kwargs)
    return experiments


def pytest_configure(config):
    config.addinivalue_line('markers', 'experiments: experiments')


@pytest.fixture(scope='function')
def alice(apphost_stubber: ApphostStubberService, megamind: Megamind, apphost: AppHost,
          test_name: str, stubber_repository: StubberRepository, experiments: Dict[str, str],
          graphs_info: GraphsInfo, megamind_nodes: Dict[str, NodeWrapper]):
    requester = MegamindRequester(megamind, apphost, apphost_stubber, test_name, experiments)
    if is_generator_mode():
        return MitGenerator(requester, stubber_repository, graphs_info, set(megamind_nodes.keys()))
    return MitRunner(requester)
