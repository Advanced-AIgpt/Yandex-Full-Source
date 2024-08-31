import pytest
from .common import Cuttlefish, create_grpc_request
from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.python.testing.items import get_answer_item
from alice.cuttlefish.library.protos.audio_pb2 import TAudio, TAudioChunk, TEndStream, TEndSpotter
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TRequestContext
from alice.cuttlefish.library.protos.store_audio_pb2 import TStoreAudioResponse
from apphost.lib.proto_answers.http_pb2 import THttpRequest, THttpResponse


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
class TestStoreAudioPre:
    @pytest.mark.parametrize("diff_mode", [True, False])
    def test_single_chunk(self, cuttlefish: Cuttlefish, diff_mode):
        req_ctx = TRequestContext(
            AudioOptions=TAudioOptions(
                Format="audio/opus",
            ),
            Header=TRequestContext.THeader(
                SessionId="sessionId",
                MessageId="messageId",
                StreamId=13,
            ),
        )
        if diff_mode:
            req_ctx.ExpFlags.update(
                {
                    "log_spotter_diff": "1",
                }
            )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": req_ctx,
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"Derevnya Durakov - Pesnya",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1

        if diff_mode:
            path = "/upload-speechbase/test_sessionId_messageId_13.opus?expire=259200s"
        else:
            path = "/upload-speechbase/sessionId_messageId_13.opus?expire=19008000s"

        mds_http_req = get_answer_item(response, ItemTypes.MDS_STORE_STREAM_HTTP_REQUEST, proto=THttpRequest)
        assert mds_http_req.Method == THttpRequest.Post
        assert mds_http_req.Path == path
        assert mds_http_req.Content == b"Derevnya Durakov - Pesnya"

    def test_multiple_chunks(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audo/x-mpeg",
                            ),
                            Header=TRequestContext.THeader(
                                SessionId="sessionId",
                                MessageId="messageId",
                                StreamId=99,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"ONE! ",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"TWO!! ",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"THREE!!! ",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"FOUR!!!!",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 1

        mds_http_req = get_answer_item(response, ItemTypes.MDS_STORE_STREAM_HTTP_REQUEST, proto=THttpRequest)
        assert mds_http_req.Method == THttpRequest.Post
        assert mds_http_req.Path == "/upload-speechbase/sessionId_messageId_99.ogg?expire=19008000s"
        assert mds_http_req.Content == b"ONE! TWO!! THREE!!! FOUR!!!!"

    def test_wav_header(self, cuttlefish: Cuttlefish):
        data = b"Bomfunk MC's: Freestyler"

        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/pcm-8",
                            ),
                            Header=TRequestContext.THeader(
                                SessionId="sessionId",
                                MessageId="messageId",
                                StreamId=1111,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=data,
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ],
            ),
        )

        def to_bytes(value, num_bytes):
            return value.to_bytes(num_bytes, byteorder="little")

        expected_content = b"RIFF"
        expected_content += to_bytes(len(data) + 36, 4)
        expected_content += b"WAVEfmt "
        expected_content += to_bytes(16, 4)
        expected_content += to_bytes(1, 2)
        expected_content += to_bytes(1, 2)  # mono channel
        expected_content += to_bytes(8000, 4)  # sample rate
        expected_content += to_bytes(8000 * 2 * 1, 4)  # byte rate
        expected_content += to_bytes(2 * 1, 2)  # block align
        expected_content += to_bytes(16, 2)  # bits per sample
        expected_content += b"data"
        expected_content += to_bytes(len(data), 4)  # subchunk size
        expected_content += data

        assert len(response.Answers) == 1

        mds_http_req = get_answer_item(response, ItemTypes.MDS_STORE_STREAM_HTTP_REQUEST, proto=THttpRequest)
        assert mds_http_req.Method == THttpRequest.Post
        assert mds_http_req.Path == "/upload-speechbase/sessionId_messageId_1111.wav?expire=19008000s"
        assert mds_http_req.Content == expected_content

    def test_spotter_and_stream(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_PRE,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.REQUEST_CONTEXT,
                        "data": TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
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
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"Hello, Alice",
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
                                Data=b"Tell me ",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(
                            Chunk=TAudioChunk(
                                Data=b"random number",
                            ),
                        ),
                    },
                    {
                        "type": ItemTypes.AUDIO,
                        "data": TAudio(EndStream=TEndStream()),
                    },
                ],
            ),
        )

        assert len(response.Answers) == 2

        mds_http_req = get_answer_item(response, ItemTypes.MDS_STORE_SPOTTER_HTTP_REQUEST, proto=THttpRequest)
        assert mds_http_req.Method == THttpRequest.Post
        assert mds_http_req.Path == "/upload-speechbase/sessionId_messageId_13_spotter.opus?expire=19008000s"
        assert mds_http_req.Content == b"Hello, Alice"

        mds_http_req = get_answer_item(response, ItemTypes.MDS_STORE_STREAM_HTTP_REQUEST, proto=THttpRequest)
        assert mds_http_req.Method == THttpRequest.Post
        assert mds_http_req.Path == "/upload-speechbase/sessionId_messageId_13.opus?expire=19008000s"
        assert mds_http_req.Content == b"Tell me random number"


class TestStoreAudioPost:
    def test_simple(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.MDS_STORE_SPOTTER_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=418,
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.STORE_AUDIO_RESPONSE_SPOTTER, proto=TStoreAudioResponse)
        assert response.StatusCode == 418
        assert response.IsSpotter

    def test_mds_failed(self, cuttlefish: Cuttlefish):
        response = cuttlefish.make_grpc_request(ServiceHandles.STORE_AUDIO_POST, create_grpc_request())
        assert get_answer_item(response, ItemTypes.NOTHING)

    def test_mds_ok(self, cuttlefish: Cuttlefish):
        key = "221/filename"

        content = f"""<?xml version="1.0" encoding="utf-8"?>
        <post obj="namespace.filename" id="81d8ba78...666dd3d1" groups="3" size="100" key="{key}">
          <complete addr="141.8.145.55:1032" path="/src/storage/8/data-0.0" group="223" status="0"/>
          <complete addr="141.8.145.116:1032" path="/srv/storage/8/data-0.0" group="221" status="0"/>
          <complete addr="141.8.145.119:1029" path="/srv/storage/5/data-0.0" group="225" status="0"/>
          <written>3</written>
        </post>
        """.encode(
            "ascii"
        )

        response = cuttlefish.make_grpc_request(
            ServiceHandles.STORE_AUDIO_POST,
            create_grpc_request(
                items=[
                    {
                        "type": ItemTypes.MDS_STORE_STREAM_HTTP_RESPONSE,
                        "data": THttpResponse(
                            StatusCode=200,
                            Content=content,
                        ),
                    }
                ],
            ),
        )

        assert len(response.Answers) == 1
        response = get_answer_item(response, ItemTypes.STORE_AUDIO_RESPONSE, proto=TStoreAudioResponse)
        assert response.StatusCode == 200
        assert response.Key == key
        assert not response.IsSpotter
