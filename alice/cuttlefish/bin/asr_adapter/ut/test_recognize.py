from .asr_adapter import AsrAdapter
import logging
import pytest
from alice.cuttlefish.library.protos.audio_pb2 import (
    TAudio,
    TBeginStream,
    TBeginSpotter,
    TAudioChunk,
    TEndStream,
    TEndSpotter,
    TMetaInfoOnly,
)
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext
from alice.cuttlefish.library.python.testing.items import get_only_answer_item, get_answer_items, create_grpc_request
from alice.cuttlefish.library.python.testing.constants import ItemTypes
from voicetech.asr.engine.proto_api.request_pb2 import TInitRequest, TRecognitionOptions, SingleUtterance
from voicetech.asr.engine.proto_api.response_pb2 import Active, EndOfUtterance


@pytest.fixture(scope="module")
def asr_adapter():
    with AsrAdapter() as x:
        yield x


class TestRecognitionWithSpotter:
    def test_recognition_with_spotter(self, asr_adapter: AsrAdapter):
        response = asr_adapter.make_grpc_request(
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/x-pcm;bit=16;rate=16000",
                            ),
                            Header=TRequestContext.THeader(
                                SessionId="sessionId",
                                MessageId="messageId",
                                StreamId=13,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(BeginStream=TBeginStream(Mime="audio/x-pcm;bit=16;rate=16000")),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            MetaInfoOnly=TMetaInfoOnly(),
                            AsrInitRequest=TInitRequest(
                                AppId="test-app-id",
                                HostName="test-host-name",
                                ClientHostname="test-client-host-name",
                                RequestId="test-request-id",
                                Uuid="test-uuid",
                                Device="test-device",
                                Topic="test-topic",
                                Lang="test-lang",
                                EouMode=SingleUtterance,
                                Mime="audio/x-pcm;bit=16;rate=16000",
                                HasSpotterPart=True,
                                RecognitionOptions=TRecognitionOptions(
                                    SpotterPhrase="алиса",
                                ),
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(BeginSpotter=TBeginSpotter()),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"Hello, Alice" * 200,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndSpotter=TEndSpotter()),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"Tell me " * int(1900 / 7),
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"random number" * int(1900 / 13),
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ],
            )
        )

        # logging.info('response:', response)
        resps = list(get_answer_items(response, ItemTypes.ASR_PROTO_RESPONSE))
        assert len(resps) == 5

        first_asr_resp = resps[0]
        logging.info(first_asr_resp)
        assert first_asr_resp.InitResponse.IsOk

        for r in resps[1:-1]:
            logging.info(r)
            assert r.AddDataResponse.IsOk
            assert r.AddDataResponse.ResponseStatus == Active

        last_add_data_resp = resps[-1]
        logging.info(last_add_data_resp)
        assert last_add_data_resp.AddDataResponse.IsOk
        assert last_add_data_resp.AddDataResponse.ResponseStatus == EndOfUtterance

        resp_spotter_validation = get_only_answer_item(response, ItemTypes.ASR_SPOTTER_VALIDATION)
        assert resp_spotter_validation.Valid
