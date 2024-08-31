from .cloud_synth import CloudSynth, SynthesizerMock

from alice.cuttlefish.library.python.testing.constants import ItemTypes

from alice.cuttlefish.library.protos.tts_pb2 import TBackendRequest
from alice.cuttlefish.library.protos.audio_pb2 import TAudio

from voicetech.asr.cloud_engine.api.speechkit.v3.service.tts.tts_service_pb2_grpc import (
    add_SynthesizerServicer_to_server,
)
from voicetech.library.proto_api.ttsbackend_pb2 import Generate

from yatest.common.network import PortManager

import pytest
import grpc

from concurrent import futures


@pytest.fixture(scope="module")
def port():
    with PortManager() as pm:
        yield pm.get_port()


@pytest.fixture(scope="module")
def synth(port: int):
    with CloudSynth(
        args=[
            "-V",
            "server.rtlog.file_stat_check_period=0.1s",
            "-V",
            "cloud.host=localhost",
            "-V",
            f"cloud.port={port}",
            "-V",
            "cloud.use_insecure_grpc=true",
            "-V",
            "cloud.token_var=CLOUD_SYNTH_TOKEN",
            "-V",
            "cloud.tokens_config_var=CLOUD_SYNTH_TOKENS_CONFIG",
        ],
        env={'CLOUD_SYNTH_TOKEN': "Api-Key the_key", 'CLOUD_SYNTH_TOKENS_CONFIG': '{"oksana": "the_key"}'},
    ) as x:
        yield x


@pytest.fixture(scope="module")
def server_mock(port: int):
    mock = SynthesizerMock()
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    add_SynthesizerServicer_to_server(mock, server)
    server.add_insecure_port(f'[::]:{port}')
    server.start()
    yield mock
    server.stop(None)


def _get_tts_backend_request(text, req_seq_no, voice="cloud_oksana"):
    return TBackendRequest(
        ReqSeqNo=req_seq_no,
        Generate=Generate(
            text=text,
            lang="ru-RU",
            voices=[Generate.WeightedParam(name=voice, weight=1.0)],
            speed=42,
            volume=11,
            mime="audio/opus",
        ),
    )


class TestCloudSynth:
    @pytest.mark.asyncio
    async def test_simple_request(self, synth: CloudSynth, server_mock: SynthesizerMock):
        async with synth.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    "tts_backend_request_cloud_synth": [
                        _get_tts_backend_request("abc", 0),
                        _get_tts_backend_request("def", 1),
                    ]
                },
                last=True,
            )

            response_chunks = await stream.read_all(timeout=10.0)
            assert len(server_mock.requests) == 2
            assert server_mock.requests[0].text == "abc"
            assert server_mock.requests[1].text == "def"
            # Check common part
            for request in server_mock.requests:
                assert request.output_audio_spec.container_audio.container_audio_type == 2
                assert len(request.hints) == 3
                assert request.hints[0].voice == "oksana"
                assert request.hints[1].speed == 42
                assert request.hints[2].volume == 11

            audio_items = []
            for chunk in response_chunks:
                for item in chunk.get_items():
                    audio_items.extend(list(chunk.get_item_datas(item_type=ItemTypes.AUDIO, proto_type=TAudio)))

            assert len(audio_items) == 10
            req_seq_no = 0
            for req_seq_no_response in (audio_items[:5], audio_items[5:]):
                for audio in req_seq_no_response:
                    assert audio.TtsBackendResponse.ReqSeqNo == req_seq_no

                assert req_seq_no_response[0].BeginStream.Mime == "audio/opus"

                assert req_seq_no_response[1].Chunk.Data == b'123'
                assert not req_seq_no_response[1].TtsBackendResponse.GenerateResponse.completed

                assert req_seq_no_response[2].Chunk.Data == b'456'
                assert not req_seq_no_response[2].TtsBackendResponse.GenerateResponse.completed

                assert req_seq_no_response[3].HasField("MetaInfoOnly")
                assert req_seq_no_response[3].TtsBackendResponse.GenerateResponse.completed

                assert req_seq_no_response[4].HasField("EndStream")

                req_seq_no += 1

    @pytest.mark.asyncio
    async def test_invalid_request(self, synth: CloudSynth, server_mock: SynthesizerMock):
        async with synth.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    "tts_backend_request_cloud_synth": [
                        _get_tts_backend_request("abc", 0, "cloud_invalid_voice"),
                        _get_tts_backend_request("def", 1, "invalid_voice"),
                    ]
                },
                last=True,
            )

            response_chunks = await stream.read_all(timeout=10.0)
            assert len(server_mock.requests) == 2
            assert server_mock.requests[0].text == "abc"
            assert server_mock.requests[1].text == "def"

            audio_items = []
            for chunk in response_chunks:
                for item in chunk.get_items():
                    audio_items.extend(list(chunk.get_item_datas(item_type=ItemTypes.AUDIO, proto_type=TAudio)))

            assert len(audio_items) == 0
