from alice.uniproxy.library.logging import Logger
Logger.init("uniproxy", is_debug=True)

import yatest.common
import yatest.common.network

import tornado.web
import tornado.gen
import tornado.ioloop
import tornado.httpserver

from tornado.websocket import websocket_connect
from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.extlog.mocks import WebHandlerLoggerMock
from alice.uniproxy.library.auth.mocks import TVMToolClientMock
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.global_state import GlobalState
import alice.uniproxy.library.processors.base_event_processor
from alice.uniproxy.library.unisystem.uniwebsocket import UniWebSocket
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.testing.mocks import (
    reset_mocks,
    FakeFlagsJsonClient,
)
import voicetech.library.proto_api.yaldi_pb2 as protos
from tornado.httpclient import HTTPRequest


class HttpServer:
    def __init__(self):
        self._app = tornado.web.Application([
            (r'/uni.ws', UniWebSocket)
        ])

        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._port_manager = None
        self.address = None
        self.port = None

    def start(self, address="", port_manager=None):
        self._port_manager = port_manager or yatest.common.network.PortManager()
        self.port = self._port_manager.get_port()
        self.address = address or "localhost"
        self._srv.listen(self.port, self.address)

    def stop(self):
        self._srv.stop()
        self._port_manager.release_port(self.port)

    def __enter__(self, *args, **kwargs):
        self.start()
        return self

    def __exit__(self, *args, **kwargs):
        self.stop()

    def ws_url(self):
        return "ws://{}:{}/uni.ws".format(self.address, self.port)


async def handle_asr(srv):
    while True:
        proto_stream = await srv.pop_proto_stream()
        if proto_stream is None:
            return

        req = await proto_stream.read_protobuf(protos.InitRequest)
        if req.topic in ("invalid-topic", "dialogeneralfast"):  # pumpkin should also fail
            await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.InvalidParams))
            proto_stream.close()
            continue

        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        chunk_count = 0
        while True:
            chunk = await proto_stream.read_protobuf(protos.AddData)
            chunk_count += 1
            if chunk.lastChunk:
                break

        await proto_stream.send_protobuf(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            messagesCount=chunk_count,
            recognition=[
                protos.Result(confidence=0.9, words=[
                    protos.Word(confidence=0.9, value="привет"),
                    protos.Word(confidence=0.9, value="алиса")
                ])
            ]
        ))


def run_with_unisystem(func):

    @run_async()
    async def test_wrap():
        with HttpServer() as srv:
            # tornado.ioloop.IOLoop.current().spawn_callback(handle_asr, asr_srv)
            with TVMToolClientMock(), WebHandlerLoggerMock() as logs:
                return await func(srv, logs)

    return test_wrap


@run_with_unisystem
async def run_testid(srv, logs):
    UniproxyCounter.init()
    UniproxyTimings.init()
    GlobalState.init()
    GlobalState.set_listening()
    GlobalState.set_ready()
    conn = await websocket_connect(HTTPRequest(srv.ws_url() + '?test-id=42&test-id=44'))
    await conn.write_message("""
{
    "event": {
        "header": {"messageId": "3aabe962-528e-41eb-884c-f5ba79004d12", "name": "SynchronizeState", "namespace": "System"},
        "payload": {"uaas_tests": [ 0 ], "auth_token": "069b6659-984b-4c5f-880e-aaedcfd84102", "disable_local_experiments": true, "uuid": "ffffffffffffffffa7dd70bc119838b7"}
    }
}
    """)
    for i in range(0, 20):
        if len(FakeFlagsJsonClient.CLIENTS) and FakeFlagsJsonClient.CLIENTS[0].fake_client:
            break
        await tornado.gen.sleep(0.5)
    assert len(FakeFlagsJsonClient.CLIENTS) == 1
    assert '&test-id=0_42_44' in FakeFlagsJsonClient.CLIENTS[0].fake_client.request.url


def test_testid(monkeypatch):
    reset_mocks()
    monkeypatch.setattr(alice.uniproxy.library.processors.base_event_processor, 'FlagsJsonClient', FakeFlagsJsonClient)
    run_testid()
