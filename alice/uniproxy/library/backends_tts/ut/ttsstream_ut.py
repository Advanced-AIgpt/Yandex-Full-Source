from .common import TtsClient, count_in_hgram_interval
from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.mocks.proto_server import ProtoServer
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_state import GlobalTags
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings, TTS_REQUEST_HGRAM
import voicetech.library.proto_api.ttsbackend_pb2 as protos
from tornado.gen import sleep
from tornado.ioloop import IOLoop

import os

Logger.init("uniproxy", is_debug=True)
UniproxyCounter.init()
UniproxyTimings.init()


@run_async()
async def test_failed_tts():
    GlobalCounter.TTS_GPU_ERR_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        client = TtsClient(params={
            "voice": "shitova.gpu",
            "disable_tts_fallback": True
        })

        # server rejects HTTP-Upgrade
        await srv.pop_proto_stream(accept=False)

        err = await client.error
        print("--- ERROR:", err)

    assert GlobalCounter.TTS_GPU_ERR_SUMM.value() == 1


@run_async()
async def test_failed_tts_after_some_audio():
    GlobalCounter.TTS_GPU_ERR_SUMM.set(0)
    GlobalCounter.TTS_GPU_FALLBACK_ERR_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        client = TtsClient(params={
            "lang": "ru",
            "voice": "shitova.gpu",
            "text": "привет человек",
        })

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/gpu/"
        generate_req = await srv_stream.read_protobuf(protos.Generate)
        assert generate_req.lang == "ru"
        assert generate_req.text == "привет человек"

        # server sends first audio chunk...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-first-chunk", completed=False
        ))
        # ...and then closes the stream
        srv_stream.close()

        # fallback must not be used if any audio has been passed to a client
        while not client.error.done():
            assert (await srv.pop_proto_stream(timeout=0.5)) is None

        # client receives first audio chunk and error
        assert (await client.pop_result()).audioData == b"hello-human-first-chunk"
        err = await client.error
        print("--- ERROR:", err)

    assert GlobalCounter.TTS_GPU_ERR_SUMM.value() == 1
    assert GlobalCounter.TTS_GPU_FALLBACK_ERR_SUMM.value() == 0


@run_async()
async def test_successfull_tts():
    GlobalCounter.TTS_RU_200_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        client = TtsClient(params={
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True
        })

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/"
        generate_req = await srv_stream.read_protobuf(protos.Generate)
        assert generate_req.lang == "ru"
        assert generate_req.text == "привет человек"
        assert generate_req.do_not_log is False

        # server sends complete audio response...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=True
        ))
        # ...and client receives it
        generate_resp = await client.pop_result()
        assert generate_resp.audioData == b"hello-human-complete-audio"
        assert generate_resp.completed is True

    assert GlobalCounter.TTS_RU_200_SUMM.value() == 1


@run_async()
async def test_failed_tts_fallback():
    GlobalCounter.TTS_GPU_VALTZ_ERR_SUMM.set(0)
    GlobalCounter.TTS_GPU_VALTZ_FALLBACK_ERR_SUMM.set(0)

    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        client = TtsClient(params={
            "lang": "ru",
            "voice": "valtz.gpu"
        })

        # server rejects any HTTP-Upgrade
        while not client.error.done():
            await srv.pop_proto_stream(accept=False, timeout=0.5)

        err = await client.error
        print("--- ERROR:", err)

    assert GlobalCounter.TTS_GPU_VALTZ_ERR_SUMM.value() == 1
    assert GlobalCounter.TTS_GPU_VALTZ_FALLBACK_ERR_SUMM.value() == 1


@run_async()
async def test_successfull_tts_fallback():
    GlobalCounter.TTS_GPU_ERR_SUMM.set(0)
    GlobalCounter.TTS_GPU_FALLBACK_200_SUMM.set(0)
    GlobalCounter.TTS_GPU_FALLBACK_ERR_SUMM.set(0)

    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        client = TtsClient(params={
            "lang": "ru",
            "voice": "shitova.gpu",
            "text": "привет человек"
        })

        # server rejects first HTTP-Upgrade (shitova.gpu)
        await srv.pop_proto_stream(accept=False, timeout=105)

        # server accepts shitova fallback
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/"
        generate_req = await srv_stream.read_protobuf(protos.Generate)
        assert generate_req.lang == "ru"
        assert generate_req.text == "привет человек"

        # server sends complete audio response
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=True
        ))

        # client receives it
        generate_resp = await client.pop_result()
        assert generate_resp.audioData == b"hello-human-complete-audio"
        assert generate_resp.completed is True

    assert GlobalCounter.TTS_GPU_ERR_SUMM.value() == 1
    assert GlobalCounter.TTS_GPU_FALLBACK_200_SUMM.value() == 1
    assert GlobalCounter.TTS_GPU_FALLBACK_ERR_SUMM.value() == 0


@run_async()
async def test_tts_request_timings():
    async def delayed_tts_server(srv):
        while True:
            srv_stream = await srv.pop_proto_stream()
            if srv_stream is None:
                break

            generate_req = await srv_stream.read_protobuf(protos.Generate)
            delay = float(generate_req.text)
            await sleep(delay)
            await srv_stream.send_protobuf(protos.GenerateResponse(audioData=b"complete-audio-data", completed=True))
            srv_stream.close()

    GlobalTimings.reset(TTS_REQUEST_HGRAM)
    GlobalTimings.reset("tts_gpu_request")
    GlobalTimings.reset("tts_gpu_valtz_request")

    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        IOLoop.current().spawn_callback(delayed_tts_server, srv)

        client = TtsClient(params={"lang": "ru", "voice": "shitova.gpu", "text": "1.0"})
        await client.pop_result()
        assert count_in_hgram_interval(TTS_REQUEST_HGRAM, GlobalTimings.get_metrics(), (1.0, 1.3)) == 1
        assert count_in_hgram_interval("tts_gpu_request", GlobalTimings.get_metrics(), (1.0, 1.3)) == 1

        client = TtsClient(params={"lang": "ru", "voice": "valtz.gpu", "text": "2.0"})
        await client.pop_result()
        assert count_in_hgram_interval(TTS_REQUEST_HGRAM, GlobalTimings.get_metrics(), (2.0, 2.3)) == 1
        assert count_in_hgram_interval("tts_gpu_valtz_request", GlobalTimings.get_metrics(), (2.0, 2.3)) == 1


@run_async()
async def test_tts_with_do_not_log():
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        TtsClient(params={
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True,
            "do_not_log": True
        })

        srv_stream = await srv.pop_proto_stream()
        generate_req = await srv_stream.read_protobuf(protos.Generate)
        assert generate_req.do_not_log is True


@run_async()
async def test_balancer_hints():
    os.environ['a_dc'] = 'myt'
    os.environ['a_ctype'] = 'prod'
    GlobalTags.init()
    params={
        "lang": "ru",
        "voice": "shitova",
        "text": "привет человек",
        "disable_tts_fallback": True,
        "balancing_mode_tts_shitova": "pre_prod"
    }
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        TtsClient(params=params)
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.headers['X-Yandex-Balancing-Hint'] == 'myt'

    os.environ['a_ctype'] = 'prestable'
    GlobalTags.init()
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        TtsClient(params=params)
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.headers['X-Yandex-Balancing-Hint'] == 'myt-pre'
