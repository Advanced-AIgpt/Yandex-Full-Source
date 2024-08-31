import logging
import os
import pytest

from alice.apphost.fixture import AppHost
from alice.hollywood.library.python.testing.it_grpc.graph import Graph
from alice.tests.library.service import Hollywood
from yatest.common.network import PortManager
import alice.hollywood.library.python.testing.it_grpc.marks as marks


logger = logging.getLogger(__name__)


class Servers:
    def __init__(self, hollywood, apphost):
        self._hollywood = hollywood
        self._apphost = apphost

    def wait_ports(self):
        for s in [self._hollywood, self._apphost]:
            s.wait_port()

    @property
    def hollywood(self):
        return self._hollywood

    @property
    def apphost(self):
        return self._apphost


def pytest_configure(config):
    for mark_name in ['mock', 'graph_name']:
        config.addinivalue_line('markers', f'{mark_name}: ...')


def choose_shard():
    # similar build variable is not propagated into  yc.context.flags so use env var
    if os.environ.get('HOLLYWOOD_SHARD'):
        return os.environ.get('HOLLYWOOD_SHARD')
    return 'all'


@pytest.fixture(scope="module")
def port_manager():
    with PortManager() as pm:
        yield pm


@pytest.fixture(scope='module')
def hollywood(port_manager, enabled_scenarios):
    logger.info(f'enabled_scenarios={enabled_scenarios}')

    shard = choose_shard()
    port = port_manager.get_port_range(None, Hollywood.port_count())
    logger.info(f'shard={shard}')
    with Hollywood(port,
                   scenarios=enabled_scenarios,
                   shard=shard,
                   wait_port=False) as service:
        yield service


@pytest.fixture(scope='module')
def apphost(port_manager):
    port = port_manager.get_port_range(None, AppHost.port_count())
    with AppHost(port, disable_all_remote_backends=True, launch_timeout_seconds=30) as service:
        yield service


@pytest.fixture(scope='module')
def servers(hollywood, apphost):
    s = Servers(hollywood=hollywood, apphost=apphost)
    s.wait_ports()
    return s


@pytest.fixture(scope='function')
def graph(request, servers, port_manager):
    mock_dict = marks.get_mock(request)
    graph_name = marks.get_graph_name(request)

    r = Graph(servers, port_manager, mock_dict, graph_name)
    r.start_mock_servers()
    yield r
    r.stop_mock_servers()
