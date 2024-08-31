from .yabio_adapter import YabioAdapter
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
from alice.cachalot.api.protos.cachalot_pb2 import TYabioContextResponse, TYabioContextSuccess
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext
from alice.cuttlefish.library.python.testing.items import get_answer_items, create_grpc_request
from alice.cuttlefish.library.python.testing.constants import ItemTypes
from voicetech.library.proto_api.yabio_pb2 import YabioRequest as TYabioInitRequest, Classify, Score


@pytest.fixture(scope="module")
def yabio_adapter():
    with YabioAdapter() as x:
        yield x


class TestYabioAdapter:
    def test_classify(self, yabio_adapter: YabioAdapter):
        response = yabio_adapter.make_grpc_request(
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
                            YabioInitRequest=TYabioInitRequest(
                                hostName="test-host-name",
                                sessionId="test-session-id",
                                uuid="test-uuid",
                                mime="audio/x-pcm;bit=16;rate=16000",
                                method=Classify,
                                group_id="test-group-id",
                                classification_tags=["children", "gender"],
                                clientHostname="test-client-host-name",
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
        resps = list(get_answer_items(response, ItemTypes.YABIO_PROTO_RESPONSE))
        assert len(resps) == 5

        first_resp = resps[0]
        logging.info(first_resp)
        assert first_resp.InitResponse.responseCode == 200

        classifications = {}
        for r in resps[1:-1]:
            logging.info(r)
            assert r.AddDataResponse.responseCode == 200
            for c in r.AddDataResponse.classification:
                classifications[c.tag] = c.classname

        assert classifications.get('children', '') == 'adult'
        assert classifications.get('gender', '') == 'male'

    def test_score(self, yabio_adapter: YabioAdapter):
        response = yabio_adapter.make_grpc_request(
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
                        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
                        "data": TYabioContextResponse(
                            Success=TYabioContextSuccess(
                                Ok=False,
                                # TODO:? Context=,
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
                            YabioInitRequest=TYabioInitRequest(
                                hostName="test-host-name",
                                sessionId="test-session-id",
                                uuid="test-uuid",
                                mime="audio/x-pcm;bit=16;rate=16000",
                                method=Score,
                                group_id="test-group-id",
                                classification_tags=["children", "gender"],
                                clientHostname="test-client-host-name",
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
        resps = list(get_answer_items(response, ItemTypes.YABIO_PROTO_RESPONSE))
        assert len(resps) == 5

        first_resp = resps[0]
        logging.info(first_resp)
        assert first_resp.InitResponse.responseCode == 200

        assert len(resps[1:-1])
        for r in resps[1:-1]:
            logging.info(r)
            assert r.AddDataResponse.responseCode == 200
            assert r.AddDataResponse.context.group_id == "test-group-id"
            assert r.AddDataResponse.context.enrolling[0].source == "fake_source"
            assert len(r.AddDataResponse.context.enrolling[0].voiceprint)

    def test_score_bad_context(self, yabio_adapter: YabioAdapter):
        response = yabio_adapter.make_grpc_request(
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
                        "type": ItemTypes.YABIO_CONTEXT_RESPONSE,
                        "data": TYabioContextResponse(
                            Success=TYabioContextSuccess(
                                Ok=True,
                                Context=b'000',  # <<< INVALID PROTOBUF CONTEXT DATA
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
                            YabioInitRequest=TYabioInitRequest(
                                hostName="test-host-name",
                                sessionId="test-session-id",
                                uuid="test-uuid",
                                mime="audio/x-pcm;bit=16;rate=16000",
                                method=Score,
                                group_id="test-group-id",
                                classification_tags=["children", "gender"],
                                clientHostname="test-client-host-name",
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
        resps = list(get_answer_items(response, ItemTypes.YABIO_PROTO_RESPONSE))
        assert len(resps) == 1

        first_resp = resps[0]
        logging.info(first_resp)
        assert first_resp.InitResponse.responseCode == 500  # internal error
