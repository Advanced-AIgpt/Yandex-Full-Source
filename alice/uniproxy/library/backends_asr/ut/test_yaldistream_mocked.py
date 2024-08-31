from alice.uniproxy.library.logging import Logger
Logger.init("uniproxy", is_debug=True)


from .common import AsrClient
from alice.uniproxy.library.backends_asr import YaldiStream
from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.mocks.proto_server import ProtoServer
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.testing.checks import match
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter, UniproxyTimings
import voicetech.library.proto_api.yaldi_pb2 as protos
from voicetech.library.proto_api.voiceproxy_pb2 import AdvancedASROptions
import tornado.gen
import tornado.ioloop
import pytest


UniproxyCounter.init()
UniproxyTimings.init()


@run_async(timeout=15)
async def test_failed_init_request():
    GlobalCounter.ASR_DIALOGGENERAL_5XX_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port}}):
        client = AsrClient(YaldiStream, params={
            "topic": "dialog-general-gpu",
            "disable_fallback": True
        })

        await srv.pop_proto_stream(accept=False)

        await client.closed
        err = await client.error
        print("--- ERROR", err)

    assert GlobalCounter.ASR_DIALOGGENERAL_GPU_5XX_SUMM.value() >= 1


@run_async(timeout=15)
async def test_client_timeouted():
    GlobalCounter.ASR_DESKTOPGENERAL_CLIENT_TIMEOUT_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "client_inactivity_timeout": 5}}):
        client = AsrClient(YaldiStream, params={
            "topic": "desktopgeneral",
            "disable_fallback": True
        })

        proto_stream = await srv.pop_proto_stream()

        # server is accepting InitRequest...
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "desktopgeneral"
        assert init_request.lang == "ru-RU"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # but client sends no data to YaldiStream

        await client.closed
        err = await client.error
        print("--- ERROR", err)

    assert GlobalCounter.ASR_DESKTOPGENERAL_CLIENT_TIMEOUT_SUMM.value() == 1


@run_async(timeout=20)
async def test_client_inactivity_timeout_from_event():
    GlobalCounter.ASR_DESKTOPGENERAL_CLIENT_TIMEOUT_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "client_inactivity_timeout": 1}}):
        client = AsrClient(YaldiStream, params={
            "topic": "desktopgeneral",
            "disable_fallback": True,
            "asr_client_inactivity_timeout": 10
        })

        proto_stream = await srv.pop_proto_stream()

        # server is accepting InitRequest...
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "desktopgeneral"
        assert init_request.lang == "ru-RU"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # client sends chunks with 3 sec. gap (too slow for inactivity timeout from config)
        for i in range(0, 3):
            client.asr_stream.add_chunk(b"small-voice-data-chunk")
            await tornado.gen.sleep(3)
        client.asr_stream.add_chunk(None)  # last chunk

        # server is answering with EOU...
        await proto_stream.send_protobuf(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            messagesCount=4,
            recognition=[
                protos.Result(confidence=0.7, words=[
                    protos.Word(confidence=0.7, value="привет"),
                    protos.Word(confidence=0.7, value="алиса")
                ])
            ]
        ))

        result = await client.pop_result()
        assert match(result, {
            "responseCode": "OK",
            "endOfUtt": True,
            "recognition": [
                {"words": [{"value": "привет"}, {"value": "алиса"}]}
            ]
        })

        await client.closed

    assert GlobalCounter.ASR_DESKTOPGENERAL_CLIENT_TIMEOUT_SUMM.value() == 0  # there were no timeout


@pytest.mark.skip()
@run_async(timeout=15)
async def test_server_timeouted():
    GlobalCounter.ASR_QUASARGENERAL_SERVER_TIMEOUT_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "yaldi_inactivity_timeout": 5}}):
        client = AsrClient(YaldiStream, params={
            "topic": "quasar-general",
            "disable_fallback": True
        })

        proto_stream = await srv.pop_proto_stream()

        # server is accepting InitRequest...
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "quasar-general"
        assert init_request.lang == "ru-RU"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # push small audio chunks to YaldiStream till it's failed by timeout
        while not client.error.done():
            client.asr_stream.add_chunk(b"voice-data")
            await tornado.gen.sleep(0.5)

        await client.closed
        err = await client.error
        print("--- ERROR", err)

    assert GlobalCounter.ASR_QUASARGENERAL_SERVER_TIMEOUT_SUMM.value() == 1


@run_async(timeout=15)
async def test_successfull():
    GlobalCounter.ASR_DIALOGMAPS_GPU_200_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port}}):
        client = AsrClient(YaldiStream, params={
            "topic": "dialogmapsgpu",
            "disable_fallback": True
        })

        proto_stream = await srv.pop_proto_stream()

        # server is accepting InitRequest...
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "dialogmapsgpu"
        assert init_request.lang == "ru-RU"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # push some data to YaldiStream...
        for i in range(0, 3):
            client.asr_stream.add_chunk(b"small-voice-data-chunk")
            await tornado.gen.sleep(0.1)
        client.asr_stream.add_chunk(None)  # last chunk

        # server is answering with EOU...
        await proto_stream.send_protobuf(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            messagesCount=4,
            recognition=[
                protos.Result(confidence=0.7, words=[
                    protos.Word(confidence=0.7, value="привет"),
                    protos.Word(confidence=0.7, value="алиса")
                ])
            ]
        ))

        result = await client.pop_result()
        assert match(result, {
            "responseCode": "OK",
            "endOfUtt": True,
            "recognition": [
                {"words": [{"value": "привет"}, {"value": "алиса"}]}
            ]
        })

        await client.closed

    assert GlobalCounter.ASR_DIALOGMAPS_GPU_200_SUMM.value() == 1


@run_async(timeout=15)
async def test_degradation_modes():
    params_uuids_and_modes = [
        # users
        (
            {
                "topic": "dialog-general-gpu",
                "disable_fallback": True
            },
            "abcabcabcfffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Auto  # default
        ), (
            {
                "topic": "quasar-general-gpu",
                "disable_fallback": True,
                "advancedASROptions": {
                    "degradation_mode": "Enable"
                }
            },
            "abcabcabcfffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Enable
        ), (
            {
                "topic": "quasar-general-gpu",
                "disable_fallback": True,
                "advancedASROptions": {
                    "degradation_mode": "Disable"
                }
            },
            "abcabcabcfffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Disable
        ),
        # robots
        (
            {
                "topic": "dialog-general-gpu",
                "disable_fallback": True
            },
            "deadbeefffffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Disable  # default
        ), (
            {
                "topic": "quasar-general-gpu",
                "disable_fallback": True,
                "advancedASROptions": {
                    "degradation_mode": "Enable"
                }
            },
            "deadbeefffffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Enable
        ), (
            {
                "topic": "quasar-general-gpu",
                "disable_fallback": True,
                "advancedASROptions": {
                    "degradation_mode": "Disable"
                }
            },
            "deadbeefffffffff833e80e69fa05e97",
            AdvancedASROptions.EDegradationMode.Disable
        ),
    ]

    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port}}):
        for params, uuid, mode in params_uuids_and_modes:
            AsrClient(YaldiStream, params=params, uuid=uuid)

            proto_stream = await srv.pop_proto_stream()

            # server is accepting InitRequest...
            init_request = await proto_stream.read_protobuf(protos.InitRequest)
            assert init_request.advanced_options.degradation_mode == mode


@run_async(timeout=15)
async def test_failed_pumpkin():
    GlobalCounter.ASR_DIALOGGENERAL_5XX_SUMM.set(0)
    GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_5XX_SUMM.set(0)

    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "backup_host": "localhost", "pumpkin_host": "localhost"}}):
        client = AsrClient(YaldiStream, params={"topic": "dialog-general"})

        await srv.pop_proto_stream(accept=False)  # original server & target topic
        await srv.pop_proto_stream(accept=False)  # backup server & target topic
        await srv.pop_proto_stream(accept=False)  # pumpkin server & fallback topic

        await client.closed
        err = await client.error
        print("--- ERROR", err)

    assert GlobalCounter.ASR_DIALOGGENERAL_5XX_SUMM.value() == 1
    assert GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_5XX_SUMM.value() == 1


@run_async(timeout=15)
async def test_successfull_pumpkin():
    GlobalCounter.ASR_DIALOGGENERAL_5XX_SUMM.set(0)
    GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_5XX_SUMM.set(0)
    GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_200_SUMM.set(0)

    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "backup_host": "localhost", "pumpkin_host": "localhost"}}):
        client = AsrClient(YaldiStream, params={"topic": "dialog-general"})
        # To enable pumpkin params mustn't CONTAIN "disable_fallback" (regardless its' value)

        # original server is rejecting InitRequest for target topic...
        proto_stream = await srv.pop_proto_stream()
        assert proto_stream.uri == "/ru-ru/dialogeneral/"
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "dialog-general"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.InternalError))

        # backup server is rejecting HTTP Upgrade with target topic...
        await srv.pop_proto_stream(accept=False)  # only this failure will be counted

        # pumpkin server is accepting fallback topic...
        proto_stream = await srv.pop_proto_stream()
        assert proto_stream.uri == "/ru-ru/dialogeneralfast/"
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "dialogeneralfast"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # push some data to YaldiStream...
        client.asr_stream.add_chunk(b"small-voice-data-chunk")
        client.asr_stream.add_chunk(None)  # last chunk

        # server is answering with EOU...
        await proto_stream.send_protobuf(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            messagesCount=2,
            recognition=[]
        ))

        result = await client.pop_result()
        assert match(result, {"responseCode": "OK", "endOfUtt": True, "recognition": []})

        await client.closed

    assert GlobalCounter.ASR_DIALOGGENERAL_5XX_SUMM.value() == 1
    assert GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_5XX_SUMM.value() == 0
    assert GlobalCounter.ASR_DIALOGGENERAL_PUMPKIN_200_SUMM.value() == 1


@run_async()
async def test_partial_timings():
    topic = "quasar-general-gpu"
    timings_name = "asr_quasargeneral_gpu_partials"

    async def asr_server_behaviour(srv):
        while True:
            proto_stream = await srv.pop_proto_stream()
            if proto_stream is None:
                return

            await proto_stream.read_protobuf(protos.InitRequest)
            await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

            # send partial response on each AddData request
            while True:
                add_data_req = await proto_stream.read_protobuf(protos.AddData)
                if add_data_req.audioData:
                    delay = float(add_data_req.audioData)  # audio data contains delay in seconds
                    await tornado.gen.sleep(delay)
                await proto_stream.send_protobuf(protos.AddDataResponse(
                    responseCode=protos.OK, endOfUtt=add_data_req.lastChunk, messagesCount=1,
                    recognition=[protos.Result(confidence=0.9, words=[protos.Word(confidence=0.9, value="ой")])]
                ))
                if add_data_req.lastChunk:
                    break

    GlobalTimings.reset(timings_name)
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port, "client_inactivity_timeout": 50}}):
        tornado.ioloop.IOLoop.current().spawn_callback(asr_server_behaviour, srv)

        client = AsrClient(YaldiStream, params={"topic": topic})

        client.asr_stream.add_chunk(b"0.0")  # to start timer
        client.asr_stream.add_chunk(b"0.0")
        for _ in range(2):
            client.asr_stream.add_chunk(b"0.75")
            await tornado.gen.sleep(0.75)
        for _ in range(3):
            client.asr_stream.add_chunk(b"1.5")
            await tornado.gen.sleep(1.5)
        client.asr_stream.add_chunk(None)

        await client.closed

    hgrams = GlobalTimings.get_metrics()
    buckets = []
    for hgram in hgrams:
        if hgram[0] == timings_name + "_hgram":
            buckets = hgram[1]
            break

    def total_ge(left):
        return sum(b[1] for b in buckets if b[0] >= left)

    assert total_ge(0.0) == 6
    assert total_ge(0.75) >= 5
    assert total_ge(1.5) >= 3


@run_async(timeout=15)
async def test_trash_results():
    with ProtoServer() as srv, ConfigPatch({"asr": {"yaldi_port": srv.port}}):
        client = AsrClient(YaldiStream, params={
            "topic": "dialogmapsgpu",
            "disable_fallback": True
        })

        proto_stream = await srv.pop_proto_stream()

        # server is accepting InitRequest...
        init_request = await proto_stream.read_protobuf(protos.InitRequest)
        assert init_request.topic == "dialogmapsgpu"
        assert init_request.lang == "ru-RU"
        await proto_stream.send_protobuf(protos.InitResponse(responseCode=protos.OK))

        # push some data to YaldiStream...
        for i in range(0, 3):
            client.asr_stream.add_chunk(b"small-voice-data-chunk")
            await tornado.gen.sleep(0.1)
        client.asr_stream.add_chunk(None)  # last chunk

        # server is answering with EOU...
        await proto_stream.send_protobuf(protos.AddDataResponse(
            responseCode=protos.OK,
            endOfUtt=True,
            is_trash=True,
            messagesCount=4,
            recognition=[
                protos.Result(
                    confidence=0.7,
                    normalized="привет алиса",
                    words=[
                        protos.Word(confidence=0.7, value="привет"),
                        protos.Word(confidence=0.7, value="алиса")
                    ]
                )
            ]
        ))

        result = await client.pop_result()
        assert match(result, {
            "responseCode": "OK",
            "endOfUtt": True,
            "isTrash": True,
            "recognition": [
                {
                    "normalized": "привет алиса",
                }
            ]
        })

        await client.closed
