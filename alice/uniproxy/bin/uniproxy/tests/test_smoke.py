from .common import UniProxyProcess, Events
from .run import run_async
import tornado.gen
import tornado.httpclient
import tornado.ioloop
import json
import datetime
import pytest

from alice.uniproxy.library.testing.checks import match


@pytest.fixture(scope="module")
def uniproxy():
    with UniProxyProcess() as x:
        yield x


@run_async
async def test_ping(uniproxy):
    client = tornado.httpclient.AsyncHTTPClient()
    resp = await client.fetch(uniproxy.url + "/ping", raise_error=False)
    assert resp.code == 200
    assert resp.body == b"pong"


@run_async
async def test_unidemo(uniproxy):
    client = tornado.httpclient.AsyncHTTPClient()
    resp = await client.fetch(uniproxy.url + "/unidemo.html", raise_error=False)
    assert resp.code == 200


@run_async
async def test_system_echo(uniproxy):
    ws_conn = await uniproxy.ws_connect()

    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState()))

    await ws_conn.write_message(json.dumps(Events.System.EchoRequest()))
    echo_response = json.loads(await ws_conn.read_message())
    assert match(echo_response, {
        "directive": {
            "header": {"namespace": "System", "name": "EchoResponse"},
            "payload": {}
        }
    })


@run_async
async def test_synchronize_state_response_new_speechkit(uniproxy):
    # for speechkitVersion >= 4.6.0 it must be SynchronizeStateResponse
    for ver in ("4.6.0", "4.6.1", "4.109.842"):
        ws_conn = await uniproxy.ws_connect()
        await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(speechkit_version=ver)))
        response = json.loads(await ws_conn.read_message())
        assert match(response, {
            "directive": {
                "header": {"namespace": "System", "name": "SynchronizeStateResponse"},
                "payload": {}
            }
        })
        with pytest.raises(tornado.gen.TimeoutError):
            await tornado.gen.with_timeout(datetime.timedelta(seconds=2), ws_conn.read_message())


@run_async
async def test_synchronize_state_response_old_speechkit(uniproxy):
    # for speechkitVersion < 4.6.0 it must not be SynchronizeStateResponse
    ws_conn = await uniproxy.ws_connect()
    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(speechkit_version="4.5.999")))
    with pytest.raises(tornado.gen.TimeoutError):
        await tornado.gen.with_timeout(datetime.timedelta(seconds=10), ws_conn.read_message())


@run_async
async def test_synchronize_state_response_messenger(uniproxy):
    # for Messenger it must be Messenger.SynchronizeStateResponse
    ws_conn = await uniproxy.ws_connect()
    await ws_conn.write_message(json.dumps(Events.System.SynchronizeState(speechkit_version="4.6.1", is_mssngr=True)))
    response = json.loads(await ws_conn.read_message())
    assert match(response, {
        "directive": {
            "header": {"namespace": "Messenger", "name": "SynchronizeStateResponse"},
            "payload": {}
        }
    })
    with pytest.raises(tornado.gen.TimeoutError):
        await tornado.gen.with_timeout(datetime.timedelta(seconds=2), ws_conn.read_message())


@run_async
async def test_unistat_counters_number(uniproxy):
    client = tornado.httpclient.AsyncHTTPClient()

    resp = await client.fetch(uniproxy.url + "/unistat", raise_error=False)
    assert resp.code == 200

    counters = json.loads(resp.body.decode("utf-8"))
    assert len(counters) < 2000
