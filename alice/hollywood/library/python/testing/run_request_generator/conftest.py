import pytest

from alice.hollywood.fixture import Hollywood
from alice.megamind.fixture import Megamind


class Servers:
    def __init__(self, megamind, bass_server, scenario_runtime, apphost):
        self._megamind = megamind
        self._bass_server = bass_server
        self._scenario_runtime = scenario_runtime
        self._apphost = apphost

    def wait_ports(self):
        for s in [self._megamind, self._bass_server,
                  self._scenario_runtime, self._apphost]:
            s.wait_port()

    @property
    def megamind(self):
        return self._megamind

    @property
    def bass_server(self):
        return self._bass_server

    @property
    def scenario_runtime(self):
        return self._scenario_runtime

    @property
    def apphost(self):
        return self._apphost


@pytest.fixture(scope='module')
def scenario_runtime(port_manager):
    port = port_manager.get_port_range(None, Hollywood.port_count())
    with Hollywood(port) as service:
        yield service


@pytest.fixture(scope='module')
def megamind(port_manager):
    port = port_manager.get_port_range(None, Megamind.port_count())
    with Megamind(port) as server:
        yield server


@pytest.fixture(scope='module')
def servers(megamind, bass_server, scenario_runtime, apphost):
    s = Servers(megamind, bass_server, scenario_runtime, apphost)
    s.wait_ports()
    return s
