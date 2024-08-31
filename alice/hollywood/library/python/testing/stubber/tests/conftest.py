import pytest

from yatest.common.network import PortManager


@pytest.fixture(scope="session", autouse=True)
def freeze_requests_user_agent():
    import requests.utils
    requests.utils.__version__ = "2.25.1"


@pytest.fixture(scope="module")
def port_manager():
    with PortManager() as pm:
        yield pm


@pytest.fixture(scope="function")
def port(port_manager):
    return port_manager.get_port()
