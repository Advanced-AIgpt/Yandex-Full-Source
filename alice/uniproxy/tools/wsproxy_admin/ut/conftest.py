from alice.uniproxy.tools.wsproxy_admin import WsproxyArgparser
import pytest


@pytest.fixture
def argparser():
    yield WsproxyArgparser()
