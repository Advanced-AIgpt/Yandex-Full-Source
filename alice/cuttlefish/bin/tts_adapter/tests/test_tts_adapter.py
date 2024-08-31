from .tts_adapter import TtsAdapter

from alice.cuttlefish.library.protos.audio_pb2 import TAudio, TBeginStream, TAudioChunk, TEndStream
from alice.cuttlefish.library.protos.tts_pb2 import TBackendRequest, TBackendResponse

from alice.cuttlefish.library.python.testing.constants import ItemTypes

from voicetech.library.proto_api.ttsbackend_pb2 import Generate, GenerateResponse

import pytest
import asyncio
import os
import time
import urllib


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def tts_adapter():
    with TtsAdapter(
        args=[
            "-V",
            "server.rtlog.file_stat_check_period=0.1s",
        ]
    ) as x:
        yield x


# -------------------------------------------------------------------------------------------------
def _reformat_guid(guid):
    guid = guid.replace("-", "")
    return f"{guid[:8]}-{guid[8:12]}-{guid[12:16]}-{guid[16:20]}-{guid[20:]}"


def _get_item_type_with_id(prefix, id):
    return f"{prefix}{id}"


def _get_tts_backend_request(chunked=False, req_seq_no=42):
    return TBackendRequest(
        Generate=Generate(
            mime="audio/opus",
            text="test",
            lang="ru",
            chunked=chunked,
        ),
        ReqSeqNo=req_seq_no,
    )


def _get_tts_backend_response(completed, req_seq_no=42):
    return TBackendResponse(
        GenerateResponse=GenerateResponse(
            completed=completed,
        ),
        ReqSeqNo=req_seq_no,
    )


def _get_begin_stream(mime, req_seq_no=42):
    return TAudio(
        BeginStream=TBeginStream(
            Mime=mime,
        ),
        TtsBackendResponse=TBackendResponse(
            ReqSeqNo=req_seq_no,
        ),
    )


def _get_audio_chunk(data, tts_backend_response=None):
    audio = TAudio(
        Chunk=TAudioChunk(
            Data=data,
        ),
    )

    if tts_backend_response is not None:
        audio.TtsBackendResponse.CopyFrom(tts_backend_response)

    return audio


def _get_end_stream(req_seq_no=42):
    return TAudio(
        EndStream=TEndStream(),
        TtsBackendResponse=TBackendResponse(
            ReqSeqNo=req_seq_no,
        ),
    )


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except asyncio.exceptions.TimeoutError:
        pass


async def _do_and_check_simple_tts_backend_request(
    stream, tts_backend_request_item_type=ItemTypes.TTS_BACKEND_REQUEST, req_seq_no=227
):
    stream.write_items(
        {
            tts_backend_request_item_type: [_get_tts_backend_request(chunked=False, req_seq_no=req_seq_no)],
        },
        last=True,
    )

    response_chunks = await stream.read_all(timeout=1.0)
    expected_audio_items_per_req_seq_no = {
        req_seq_no: [
            _get_begin_stream("audio/opus", req_seq_no=req_seq_no),
            _get_audio_chunk(
                b"fake_audio", tts_backend_response=_get_tts_backend_response(True, req_seq_no=req_seq_no)
            ),
            _get_end_stream(req_seq_no=req_seq_no),
        ],
    }
    _check_response(
        response_chunks,
        expected_audio_items_per_req_seq_no,
    )


def _check_response(
    response_chunks,
    expected_audio_items_per_req_seq_no,
    check_last_nothing=True,
):
    assert len(response_chunks) > 0, "At least one response chunk expected"

    expected_audio_items_ptr_per_req_seq_no = {
        req_seq_no: 0 for req_seq_no in expected_audio_items_per_req_seq_no.keys()
    }

    non_nothing_chunk_count = len(response_chunks) - int(check_last_nothing)
    for chunk in response_chunks[:non_nothing_chunk_count]:
        audio_items = list(chunk.get_item_datas(item_type=ItemTypes.AUDIO, proto_type=TAudio))
        assert len(list(chunk.get_items())) == len(audio_items), "Only audio expected"

        for audio_item in audio_items:
            assert audio_item.HasField(
                "TtsBackendResponse"
            ), "All audio items must have TtsBackendResponse to match them by ReqSeqNo"

            req_seq_no = audio_item.TtsBackendResponse.ReqSeqNo
            assert (
                req_seq_no in expected_audio_items_per_req_seq_no
            ), f"{req_seq_no} not found in {expected_audio_items_per_req_seq_no.keys()}"

            ptr = expected_audio_items_ptr_per_req_seq_no[req_seq_no]
            assert ptr < len(
                expected_audio_items_per_req_seq_no[req_seq_no]
            ), f"Too many audio items with ReqSeqNo={req_seq_no} in response"

            expected_audio_item = expected_audio_items_per_req_seq_no[req_seq_no][ptr]
            expected_audio_items_ptr_per_req_seq_no[req_seq_no] += 1

            if expected_audio_item.HasField("BeginStream"):
                assert audio_item.HasField("BeginStream")
                assert audio_item.BeginStream.Mime == expected_audio_item.BeginStream.Mime
            elif expected_audio_item.HasField("EndStream"):
                assert audio_item.HasField("EndStream")
            elif expected_audio_item.HasField("Chunk"):
                assert audio_item.HasField("Chunk")
                assert audio_item.Chunk.Data == expected_audio_item.Chunk.Data
            else:
                assert False, f"Unknown audio type {expected_audio_item}"

            assert (
                audio_item.TtsBackendResponse.GenerateResponse.completed
                == expected_audio_item.TtsBackendResponse.GenerateResponse.completed
            )
            assert audio_item.TtsBackendResponse.ReqSeqNo == expected_audio_item.TtsBackendResponse.ReqSeqNo

    if check_last_nothing:
        assert (
            len(list(response_chunks[-1].get_items(item_type=ItemTypes.NOTHING))) == 1
        ), "'!nothing' not found at the end of stream"
        assert (
            len(list(response_chunks[-1].get_items())) == 1
        ), f"Only '{ItemTypes.NOTHING}' expected at the end of stream"

    for req_seq_no, ptr in expected_audio_items_ptr_per_req_seq_no.items():
        expected_ptr = len(expected_audio_items_per_req_seq_no[req_seq_no])
        assert (
            expected_ptr == ptr
        ), f"Only {ptr} items in output, but {expected_ptr} expected for audio items with ReqSeqNo={req_seq_no}"


# -------------------------------------------------------------------------------------------------
class TestTtsAdapter:
    @pytest.mark.asyncio
    async def test_logs_rotation(self, tts_adapter: TtsAdapter):
        async def make_request_and_check_logs(guid):
            async with tts_adapter.create_apphost_grpc_stream(guid=guid) as stream:
                await _do_and_check_simple_tts_backend_request(stream)

            eventlog = [event for event in tts_adapter.get_eventlog(from_beginning=True)]
            tts_cache_callbacks_frames = 0
            for event in eventlog:
                if event["EventBody"]["Type"] == "TtsCallbacksFrame":
                    assert event["EventBody"]["Fields"]["GUID"] == _reformat_guid(guid)
                    tts_cache_callbacks_frames += 1

            assert tts_cache_callbacks_frames == 1

            rtlog = [r for r in tts_adapter.get_rtlog(from_beginning=True)]

            tts_cache_callbacks_rtlog = 0
            for rec in rtlog[:10]:
                print(f"REC: {rec}")
                msg = rec["EventBody"]["Fields"].get("Message", "")
                if msg.startswith("TtsCallbacksFrame:"):
                    assert f"{_reformat_guid(guid)}" in msg
                    tts_cache_callbacks_rtlog += 1
            assert tts_cache_callbacks_rtlog == 1

        await make_request_and_check_logs("01234567-89abcdef-01234567-89abcdef")

        # rotate eventlog
        os.rename(tts_adapter.eventlog_path, tts_adapter.eventlog_path + ".1")
        os.rename(tts_adapter.rtlog_path, tts_adapter.rtlog_path + ".1")
        urllib.request.urlopen(f"http://{tts_adapter.http_endpoint}/admin?action=reopenlog")
        time.sleep(0.2)  # double rtlog's check period to be sure in rotation

        await make_request_and_check_logs("aaaaaaaa-bbbbbbbb-cccccccc-dddddddd")

    @pytest.mark.asyncio
    @pytest.mark.parametrize(
        "tts_backend_request_item_type",
        [
            ItemTypes.TTS_BACKEND_REQUEST,
            _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_shitova.gpu"),
        ],
    )
    async def test_simple_request(self, tts_adapter: TtsAdapter, tts_backend_request_item_type):
        async with tts_adapter.create_apphost_grpc_stream() as stream:
            await _do_and_check_simple_tts_backend_request(
                stream, tts_backend_request_item_type=tts_backend_request_item_type
            )

    @pytest.mark.asyncio
    async def test_requests_in_different_chunks(self, tts_adapter: TtsAdapter):
        REQ_SEQ_NO_FIRST = 93853
        REQ_SEQ_NO_LAST = 96853

        async with tts_adapter.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_BACKEND_REQUEST: [
                        _get_tts_backend_request(chunked=False, req_seq_no=REQ_SEQ_NO_FIRST),
                    ],
                },
                last=False,
            )

            # Secret knowledge of fake implementation
            # There are will be three chunks
            # It is stable behavior in not chunked mode
            response_chunks = []
            for i in range(3):
                chunk = await stream.read(timeout=1.0)
                response_chunks.append(chunk)
            expected_audio_items_per_req_seq_no = {
                REQ_SEQ_NO_FIRST: [
                    _get_begin_stream("audio/opus", req_seq_no=REQ_SEQ_NO_FIRST),
                    _get_audio_chunk(
                        b"fake_audio", tts_backend_response=_get_tts_backend_response(True, req_seq_no=REQ_SEQ_NO_FIRST)
                    ),
                    _get_end_stream(req_seq_no=REQ_SEQ_NO_FIRST),
                ],
            }
            _check_response(
                response_chunks,
                expected_audio_items_per_req_seq_no,
                check_last_nothing=False,
            )

            stream.write_items(
                {
                    ItemTypes.TTS_BACKEND_REQUEST: [
                        _get_tts_backend_request(chunked=False, req_seq_no=REQ_SEQ_NO_LAST),
                    ],
                },
                last=True,
            )

            # Read all till end of stream
            response_chunks = await stream.read_all(timeout=1.0)
            expected_audio_items_per_req_seq_no = {
                REQ_SEQ_NO_LAST: [
                    _get_begin_stream("audio/opus", req_seq_no=REQ_SEQ_NO_LAST),
                    _get_audio_chunk(
                        b"fake_audio", tts_backend_response=_get_tts_backend_response(True, req_seq_no=REQ_SEQ_NO_LAST)
                    ),
                    _get_end_stream(req_seq_no=REQ_SEQ_NO_LAST),
                ],
            }
            _check_response(
                response_chunks,
                expected_audio_items_per_req_seq_no,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("subrequest_count", [1, 10])
    async def test_chunked(self, tts_adapter: TtsAdapter, subrequest_count):
        REQ_SEQ_NO_START = 23451

        async with tts_adapter.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_BACKEND_REQUEST: [
                        _get_tts_backend_request(chunked=True, req_seq_no=req_seq_no)
                        for req_seq_no in range(REQ_SEQ_NO_START, REQ_SEQ_NO_START + subrequest_count)
                    ],
                },
                last=True,
            )

            response_chunks = await stream.read_all(timeout=20.0)
            expected_audio_items_per_req_seq_no = {
                req_seq_no: [
                    _get_begin_stream("audio/opus", req_seq_no=req_seq_no),
                ]
                + [
                    _get_audio_chunk(
                        b"fake_audio", tts_backend_response=_get_tts_backend_response(False, req_seq_no=req_seq_no)
                    )
                    for i in range(9)
                ]
                + [
                    _get_audio_chunk(
                        b"fake_audio", tts_backend_response=_get_tts_backend_response(True, req_seq_no=req_seq_no)
                    ),
                    _get_end_stream(req_seq_no=req_seq_no),
                ]
                for req_seq_no in range(REQ_SEQ_NO_START, REQ_SEQ_NO_START + subrequest_count)
            }

            _check_response(
                response_chunks,
                expected_audio_items_per_req_seq_no,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize("subrequest_count", [1, 10])
    async def test_cancel(self, tts_adapter: TtsAdapter, subrequest_count):
        REQUEST_COUNT = 10

        for i in range(REQUEST_COUNT):
            async with tts_adapter.create_apphost_grpc_stream() as stream:
                stream.write_items(
                    {
                        ItemTypes.TTS_BACKEND_REQUEST: [
                            _get_tts_backend_request(chunked=True, req_seq_no=req_seq_no)
                            for req_seq_no in range(subrequest_count)
                        ],
                    },
                    last=False,
                )
                await asyncio.sleep(0.1)
                stream.cancel()

        # Check that tts_adapter still alive
        async with tts_adapter.create_apphost_grpc_stream() as stream:
            await _do_and_check_simple_tts_backend_request(stream)

    @pytest.mark.asyncio
    async def test_invalid_request(self, tts_adapter: TtsAdapter):
        async with tts_adapter.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    # Invalid protobuf in request
                    ItemTypes.TTS_BACKEND_REQUEST: [TBackendResponse()],
                },
                last=False,
            )
            await _check_no_output(stream, "No response for invalid request excepted for now")

        # Check that tts_adapter still alive
        async with tts_adapter.create_apphost_grpc_stream() as stream:
            await _do_and_check_simple_tts_backend_request(stream)

    @pytest.mark.asyncio
    async def test_subrequests_with_same_req_seq_no(self, tts_adapter: TtsAdapter):
        REQ_SEQ_NO = 93853

        async with tts_adapter.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_BACKEND_REQUEST: [
                        _get_tts_backend_request(chunked=False, req_seq_no=REQ_SEQ_NO),
                        # A lot of same requests with chunked mode (output of chunked mode is different from output of not chunked mode)
                        # All of them must be ignored
                        _get_tts_backend_request(chunked=True, req_seq_no=REQ_SEQ_NO),
                        _get_tts_backend_request(chunked=True, req_seq_no=REQ_SEQ_NO),
                    ],
                },
                last=True,
            )

            response_chunks = await stream.read_all(timeout=1.0)
            expected_audio_items_per_req_seq_no = {
                REQ_SEQ_NO: [
                    _get_begin_stream("audio/opus", req_seq_no=REQ_SEQ_NO),
                    _get_audio_chunk(
                        b"fake_audio", tts_backend_response=_get_tts_backend_response(True, req_seq_no=REQ_SEQ_NO)
                    ),
                    _get_end_stream(req_seq_no=REQ_SEQ_NO),
                ],
            }
            _check_response(
                response_chunks,
                expected_audio_items_per_req_seq_no,
            )
