from .common import UniProxyProcess
from .run import run_async
import tornado.httpclient
import pytest


@pytest.fixture(scope="module")
def uniproxy():
    with UniProxyProcess(nproc=10) as x:
        yield x


@run_async
async def test_exp_http_handle(uniproxy):
    client = tornado.httpclient.AsyncHTTPClient()

    # check original share = 0.75
    resp = await client.fetch(uniproxy.url + "/exp?id=test-experiment-1", raise_error=False)
    assert resp.code == 200
    assert resp.body.decode("utf-8").strip() == "test-experiment-1 share=0.75"

    # set new share = 0.25
    resp = await client.fetch(uniproxy.url + "/exp?id=test-experiment-1&share=0.25", raise_error=False)
    assert resp.code == 200

    # check new share a few times to get responses from different processes
    for i in range(10):
        resp = await client.fetch(uniproxy.url + "/exp?id=test-experiment-1", raise_error=False)
        assert resp.code == 200
        assert resp.body.decode("utf-8").strip() == "test-experiment-1 share=0.25"
