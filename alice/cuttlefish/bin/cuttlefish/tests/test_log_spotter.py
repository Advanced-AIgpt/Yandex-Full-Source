import pytest
from .common import Cuttlefish, create_grpc_request
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item
from alice.cuttlefish.library.protos.audio_pb2 import TAudio, TAudioChunk, TEndStream
from alice.cuttlefish.library.protos.events_pb2 import TDirective


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
class TestLogSpotter:
    def test_no_streamcontrol(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.LOG_SPOTTER,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(Chunk=TAudioChunk(Data=b"aaaa")),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(Chunk=TAudioChunk(Data=b"bbbb")),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(Chunk=TAudioChunk(Data=b"cccc")),
                    },
                ]
            ),
        )

        assert len(response.Answers) == 1
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_has_streamcontrol(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.LOG_SPOTTER,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(Chunk=TAudioChunk(Data=b"aaaa")),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ]
            ),
        )

        assert len(response.Answers) == 1
        directive = get_answer_item(response, ItemTypes.DIRECTIVE, proto=TDirective)
        assert directive.HasField("LogAckResponse")
