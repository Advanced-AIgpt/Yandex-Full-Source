import pytest
import logging
from .auth_servers import wait_for_fanout_mock, wait_for_yamb_mock
from .subway_client import SubwayClientMock
from .tvm_client import TvmClientMock
from .websocket import WebsocketMock


__logger = logging.getLogger("mocks")
__logger.setLevel(logging.DEBUG)


def DBG(*args, **kw):
     __logger.debug(*args, **kw)


def INFO(*args, **kw):
    __logger.info(*args, **kw)


@pytest.fixture(scope="module")
def subway_client_mock():
    import subway.client

    logging.debug("subway-client mocked")
    orig = subway.client.subway_client()
    subway.client._g_client = SubwayClientMock()
    yield subway.client._g_client

    logging.debug("subway-client unmocked")
    subway.client._g_client = orig


@pytest.fixture(scope="module")
def tvm_client_mock():
    import alice.uniproxy.library.auth.tvm2 as tvm2

    logging.debug("tvm-client mocked")
    orig = tvm2.tvm_client()
    tvm2._g_tvm_tool_client = TvmClientMock()
    yield tvm2._g_tvm_tool_client

    logging.debug("tvm-client unmocked")
    tvm2._g_tvm_tool_client = orig


@pytest.fixture(scope="module")
def no_lass():
    from alice.uniproxy.library.settings import config

    logging.debug("lass disabled")
    orig = config.get("laas", {})
    config.set_by_path("laas", {})
    yield

    logging.debug("lass restored")
    config.set_by_path("laas", orig)
