from alice.uniproxy.library.logging import Logger
Logger.init("uniproxy", is_debug=True)

import json
import yatest.common
import yatest.common.network

import tornado.web
import tornado.gen
import tornado.ioloop
import tornado.httpserver

from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.mocks.proto_server import ProtoServer
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.testing.http_request import http_request
from alice.uniproxy.library.extlog.mocks import WebHandlerLoggerMock
from alice.uniproxy.library.auth.mocks import TVMToolClientMock
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
from alice.uniproxy.library.web_handlers import AsrHandler
import voicetech.library.proto_api.yaldi_pb2 as protos


UniproxyCounter.init()
UniproxyTimings.init()


# ====================================================================================================================
class HttpServer:
    def __init__(self):
        self._app = tornado.web.Application([
            (r'/asr', AsrHandler)
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

    async def request(self, **kwargs):
        return await http_request(host=self.address, port=self.port, **kwargs)


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


def run_with_mocked_asr(func):

    @run_async()
    async def test_wrap():
        with HttpServer() as srv, ProtoServer() as asr_srv, ConfigPatch({"asr": {
            "yaldi_port": asr_srv.port,
            "yaldi_host": asr_srv.address,
            "backup_host": asr_srv.address,
            "pumpkin_host": asr_srv.address
        }}):
            tornado.ioloop.IOLoop.current().spawn_callback(handle_asr, asr_srv)
            with TVMToolClientMock(), WebHandlerLoggerMock() as logs:
                return await func(srv, logs)

    return test_wrap


def get_recognized_text(jresp):
    return " ".join(w["value"] for w in jresp[0]["recognition"][0]["words"])


def total_count_in_hgram(name):
    metrics = GlobalTimings.get_metrics()
    for m in metrics:
        if m[0] != f"{name}_hgram":
            continue
        return sum(b[1] for b in m[1])
    return -1


# ====================================================================================================================
@run_with_mocked_asr
async def test_with_content_type_header(srv, logs):
    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    resp = await srv.request(url="/asr", method="POST", body=audio, headers={
        "Content-Type": "audio/opus",
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 200
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["asr"] == {"topic": "quasar-general", "format": "audio/opus", "disableAntimatNormalizer": False}

    # 'Content-Type' header takes precedence  over 'format' parameter

    resp = await srv.request(url="/asr?format=pcm", method="POST", body=audio, headers={
        "Content-Type": "audio/opus",
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 200
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["asr"] == {"topic": "quasar-general", "format": "audio/opus", "disableAntimatNormalizer": False}

    resp = await srv.request(url="/asr?format=audio/opus", method="POST", body=audio, headers={
        "Content-Type": "pcm",
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 200
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["asr"] == {"topic": "quasar-general", "format": "audio/x-pcm;bit=16;rate=16000", "disableAntimatNormalizer": False}


# ====================================================================================================================
@run_with_mocked_asr
async def test_auth_by_auth_token(srv, logs):
    GlobalCounter.HANDLER_ASR_ASR_2XX_OK_SUMM.set(0)
    GlobalTimings.reset("handler_asr_time")
    GlobalTimings.reset("handler_asr_payload_size")

    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    resp = await srv.request(url="/asr", method="POST", body=audio, headers={
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 200
    assert GlobalCounter.HANDLER_ASR_ASR_2XX_OK_SUMM.value() == 1
    assert total_count_in_hgram("handler_asr_time") == 1
    assert total_count_in_hgram("handler_asr_payload_size") == 1
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["path"] == "/asr"
    assert log_record["auth_token"] == "developers-simple-key"
    assert log_record["auth"]["auth_token"] is True
    assert log_record["auth"]["service_ticket"] is False


# ====================================================================================================================
@run_with_mocked_asr
async def test_auth_by_tvm_ticket(srv, logs):
    GlobalCounter.HANDLER_ASR_ASR_2XX_OK_SUMM.set(0)
    GlobalTimings.reset("handler_asr_time")
    GlobalTimings.reset("handler_asr_payload_size")

    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    resp = await srv.request(url="/asr", method="POST", body=audio, headers={
        "X-Ya-Service-Ticket": "some-valid-service-ticket"
    })
    assert resp.code == 200
    assert GlobalCounter.HANDLER_ASR_ASR_2XX_OK_SUMM.value() == 1
    assert total_count_in_hgram("handler_asr_time") == 1
    assert total_count_in_hgram("handler_asr_payload_size") == 1
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["path"] == "/asr"
    assert log_record["auth_token"] is None
    assert log_record["auth"]["auth_token"] is False
    assert log_record["auth"]["service_ticket"] is True


# ====================================================================================================================
@run_with_mocked_asr
async def test_unauthorized(srv, logs):
    GlobalTimings.reset("handler_asr_time")
    GlobalTimings.reset("handler_asr_payload_size")

    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    # failed request (because of unauthorized)
    resp = await srv.request(url="/asr", method="POST", body=audio)
    assert resp.code == 401
    assert resp.reason == "Unauthorized"
    assert total_count_in_hgram("handler_asr_time") == 0  # request actually wasn't handled
    assert total_count_in_hgram("handler_asr_payload_size") == 1
    log_record = await logs.pop_record()
    assert log_record["status"] == 401
    assert log_record["path"] == "/asr"
    assert log_record["auth_token"] is None
    assert log_record["auth"]["auth_token"] is False
    assert log_record["auth"]["service_ticket"] is False


# ====================================================================================================================
@run_with_mocked_asr
async def test_authorized_with_appid(srv, logs):
    GlobalTimings.reset("handler_asr_time")
    GlobalTimings.reset("handler_asr_payload_size")

    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    # failed request (because of unauthorized)
    resp = await srv.request(url="/asr", method="POST", headers={
        'X-Ya-App-Id': 'b9f8b764-7921-4b1f-8e5b-a70315ed6f0b',
    }, body=audio)
    assert resp.code == 200
    assert GlobalCounter.HANDLER_ASR_ASR_2XX_OK_SUMM.value() == 1
    assert total_count_in_hgram("handler_asr_time") == 1
    assert total_count_in_hgram("handler_asr_payload_size") == 1
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["path"] == "/asr"
    assert log_record["auth_token"] is None
    assert log_record["auth"]["auth_token"] is False
    assert log_record["auth"]["service_ticket"] is False


# ====================================================================================================================
@run_with_mocked_asr
async def test_unauthorized_with_app_id(srv, logs):
    GlobalTimings.reset("handler_asr_time")
    GlobalTimings.reset("handler_asr_payload_size")

    audio = (b"A" * 32000 * 5)  # 5 seconds of PCM16/16KHz

    # failed request (because of unauthorized)
    resp = await srv.request(url="/asr", method="POST", headers={
        'X-Ya-App-Id': '40a5beea-fce7-44e6-aaec-72a93a73e630',
    }, body=audio)
    assert resp.code == 401
    assert resp.reason == "Unauthorized"
    assert total_count_in_hgram("handler_asr_time") == 0  # request actually wasn't handled
    assert total_count_in_hgram("handler_asr_payload_size") == 1
    log_record = await logs.pop_record()
    assert log_record["status"] == 401
    assert log_record["path"] == "/asr"
    assert log_record["auth_token"] is None
    assert log_record["auth"]["auth_token"] is False
    assert log_record["auth"]["service_ticket"] is False


# ====================================================================================================================
@run_with_mocked_asr
async def test_asr_chunked(srv, logs):
    total_sent = 0

    async def speaker():
        nonlocal total_sent

        total_duration = 5
        chunk_duration = 0.02
        chunk_size = int(32000 * chunk_duration)

        d = 0
        while d < total_duration:
            await tornado.gen.sleep(chunk_duration)
            yield b"A" * chunk_size
            total_sent += chunk_size
            d += chunk_duration

    resp = await srv.request(
        url="/asr",
        method="POST",
        body=speaker,
        headers={
            "X-UPRX-AUTH-TOKEN": "developers-simple-key",
            "X-UPRX-UUID": "some-uuid",
            "X-Real-Ip": "8.8.8.8"
        }
    )

    assert resp.code == 200
    jresp = json.loads(resp.body.decode("utf-8"))
    assert jresp[0]["responseCode"] == "OK"
    assert get_recognized_text(jresp) == "привет алиса"

    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["path"] == "/asr"
    assert log_record["body_size"] == total_sent
    assert log_record["auth_token"] is not None
    assert log_record["uuid"] == "some-uuid"
    assert log_record["ip"] == "8.8.8.8"
    assert log_record["auth"]["auth_token"] is True
    assert log_record["auth"]["service_ticket"] is False


# ====================================================================================================================
@run_with_mocked_asr
async def test_too_large_chunked_payload(srv, logs):

    async def speaker():
        chunk = b"A" * (256 * 1024)  # 256Kb
        while True:
            await tornado.gen.sleep(0.01)  # tornado.gen.sleep(0.01)
            yield chunk

    resp = await srv.request(url="/asr", method="POST", body=speaker, headers={
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 413
    assert resp.reason == "Payload Too Large"
    log_record = await logs.pop_record()
    assert log_record["status"] == 413


# ====================================================================================================================
@run_with_mocked_asr
async def test_with_disabled_antimat(srv, logs):
    resp = await srv.request(url="/asr?disable_antimat=y", method="POST", body=b"ABC", headers={
        "Content-Type": "audio/opus",
        "X-UPRX-AUTH-TOKEN": "developers-simple-key"
    })
    assert resp.code == 200
    log_record = await logs.pop_record()
    assert log_record["status"] == 200
    assert log_record["asr"] == {"topic": "quasar-general", "format": "audio/opus", "disableAntimatNormalizer": True}
