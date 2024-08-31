from alice.uniproxy.library.logging import Logger
Logger.init("uniproxy", is_debug=True)

import tornado.gen
import tornado.ioloop

import yatest.common
from .common import HttpServer
from alice.uniproxy.library.web_handlers import AsrWebSocket

from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.mocks.proto_server import ProtoServer
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.extlog.mocks import WebHandlerLoggerMock
from alice.uniproxy.library.auth.mocks import TVMToolClientMock
import voicetech.library.proto_api.yaldi_pb2 as protos


CLIENT_BIN_PATH = yatest.common.binary_path("voicetech/infra/tools/websocketproxy-recognizer/websocketproxy-recognizer")


# -------------------------------------------------------------------------------------------------
async def run_client(*args):
    from tornado.process import Subprocess

    proc = Subprocess([CLIENT_BIN_PATH, "--verbose"] + [str(a) for a in args], stderr=Subprocess.STREAM)
    logs = (await proc.stderr.read_until_close()).decode("utf-8")
    retcode = await proc.wait_for_exit(raise_error=False)

    print("\n--- BEGIN CLIENT LOG ---\n" + logs + "\n--- END CLIENT LOG ---")

    return retcode, logs


# -------------------------------------------------------------------------------------------------
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
                protos.Result(
                    confidence=0.9,
                    words=[
                        protos.Word(confidence=0.9, value="привет"),
                        protos.Word(confidence=0.9, value="алиса")
                    ],
                    normalized="привет алиса"
                )
            ]
        ))


# -------------------------------------------------------------------------------------------------
def run_with_mocked_asr(func):

    @run_async()
    async def test_wrap():
        with HttpServer((r'/asrsocket.ws', AsrWebSocket)) as srv, ProtoServer() as asr_srv, ConfigPatch({
            "asr": {
                "yaldi_port": asr_srv.port,
                "yaldi_host": asr_srv.address,
                "backup_host": asr_srv.address,
                "pumpkin_host": asr_srv.address
            },
            "apikeys": {
                "whitelist": [
                    "developer-simple-key"
                ]
            }
        }):
            tornado.ioloop.IOLoop.current().spawn_callback(handle_asr, asr_srv)
            with TVMToolClientMock(), WebHandlerLoggerMock() as log:
                return await func(srv, log)

    return test_wrap


# -------------------------------------------------------------------------------------------------
def create_audio(length):
    fake_audio_path = "./audio.opus"
    with open(fake_audio_path, "wb") as f:
        f.write(b"A" * length)
    return fake_audio_path


# -------------------------------------------------------------------------------------------------
@run_with_mocked_asr
async def test_asr_web_socket(srv, log):
    await tornado.gen.sleep(1)

    retcode, logs = await run_client(
        "--url", f"ws://{srv.address}:{srv.port}/asrsocket.ws",
        "--input", create_audio(10240),
        "--auth-token", "developer-simple-key"
    )
    assert retcode == 0
    assert "привет алиса" in logs

    log_record = await log.pop_record()
    assert log_record["status"] == 200
    assert log_record["path"] == "/asrsocket.ws"
    assert log_record["ip"] is not None
    assert log_record["key"] == "developer-simple-key"
    assert log_record["uuid"] == "deadbeef-a604-4155-bd1f-347c7bc9aa77"


# -------------------------------------------------------------------------------------------------
@run_with_mocked_asr
async def test_asr_web_socket_with_invalid_token(srv, log):
    await tornado.gen.sleep(1)

    retcode, logs = await run_client(
        "--url", f"ws://{srv.address}:{srv.port}/asrsocket.ws",
        "--input", create_audio(10240),
        "--auth-token", "villain-simple-key"
    )
    assert retcode != 0

    log_record = await log.pop_record()
    assert log_record["status"] == 403
    assert log_record["path"] == "/asrsocket.ws"
    assert log_record["ip"] is not None
    assert log_record["key"] == "villain-simple-key"
