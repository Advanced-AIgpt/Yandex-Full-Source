import base64
import pytest
import yatest
import zlib
from collections import defaultdict

from asyncio.exceptions import TimeoutError
from .common import Cuttlefish

from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.protos.audio_pb2 import (
    TAudio,
    TAudioChunk,
    TBeginStream,
    TEndStream,
    TMetaInfoOnly,
)
from alice.cuttlefish.library.protos.tts_pb2 import (
    TBackendResponse,
    TAggregatorRequest,
    TTimings,
    TCacheEntry,
    ECacheGetResponseStatus,
    TCacheGetResponse,
    TCacheSetRequest,
    TAggregatorAudioMetaInfo,
)

from voicetech.library.proto_api.ttsbackend_pb2 import GenerateResponse

from apphost.lib.proto_answers.http_pb2 import THeader, THttpResponse


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
COMPARISON_EPS = 10**-2


def _get_item_type_with_id(prefix, id):
    return f"{prefix}{id}"


def _get_aggregator_http_response_audio_source(response_item_type):
    return TAggregatorRequest.TAudioSource(
        HttpResponse=TAggregatorRequest.TAudioSource.THttpResponse(ItemType=response_item_type),
    )


def _get_aggregator_cache_get_response_audio_source():
    return TAggregatorRequest.TAudioSource(
        CacheGetResponse=TAggregatorRequest.TAudioSource.TCacheGetResponse(),
    )


def _get_aggregator_protocol_audio_audio_source(req_seq_no):
    return TAggregatorRequest.TAudioSource(
        Audio=TAggregatorRequest.TAudioSource.TAudio(ReqSeqNo=req_seq_no),
    )


def _get_aggregator_audio_part(audio_sources, cache_key="empty"):
    return TAggregatorRequest.TAudioPart(
        AudioSources=audio_sources,
        CacheKey=cache_key,
    )


def _get_aggregator_audio_meta_info(first_chunk_audio_source=None, current_chunk_audio_source=None):
    assert (
        first_chunk_audio_source is None or current_chunk_audio_source is None
    ), "Both first_chunk_audio_source and current_chunk_audio_source provided"

    aggregator_audio_meta_info = TAggregatorAudioMetaInfo()

    if first_chunk_audio_source is not None:
        aggregator_audio_meta_info.FirstChunkAudioSource = first_chunk_audio_source

    if current_chunk_audio_source is not None:
        aggregator_audio_meta_info.CurrentChunkAudioSource = current_chunk_audio_source

    return aggregator_audio_meta_info


def _get_tts_backend_timings(subtimings_cnt, time_shift=0.0):
    return GenerateResponse.Timings(
        timings=[
            GenerateResponse.Timings.Timing(
                time=500 * i + time_shift,
                phoneme=f"phoneme_{i}",
            )
            for i in range(subtimings_cnt)
        ]
    )


def _get_tts_backend_timings_multi(timings_cnt, subtimings_cnt, time_shift=0.0):
    return [_get_tts_backend_timings(subtimings_cnt, time_shift) for i in range(timings_cnt)]


def _get_tts_generate_response(completed=False, response_code=None, timings=None, duration=None, message=None):
    tts_generate_response = GenerateResponse(
        completed=completed,
    )

    if response_code is not None:
        tts_generate_response.responseCode = response_code

    if timings is not None:
        tts_generate_response.timings.CopyFrom(timings)

    if duration is not None:
        tts_generate_response.duration = duration

    if message is not None:
        tts_generate_response.message = message

    return tts_generate_response


def _get_tts_backend_response(req_seq_no=42, tts_generate_response=None):
    tts_backend_response = TBackendResponse(
        ReqSeqNo=req_seq_no,
    )

    if tts_generate_response is not None:
        tts_backend_response.GenerateResponse.CopyFrom(tts_generate_response)

    return tts_backend_response


def _get_http_response(content, status_code=200, content_type="random"):
    return THttpResponse(
        StatusCode=status_code,
        Headers=[
            THeader(
                Name="Content-Type",
                Value=content_type,
            ),
        ],
        Content=content,
    )


def _add_meta_info_to_audio(audio, tts_backend_response=None, aggregator_audio_meta_info=None):
    assert (
        tts_backend_response is None or aggregator_audio_meta_info is None
    ), "Both tts_backend_response and aggregator_audio_meta_info provided"

    if tts_backend_response is not None:
        audio.TtsBackendResponse.CopyFrom(tts_backend_response)

    if aggregator_audio_meta_info is not None:
        audio.TtsAggregatorAudioMetaInfo.CopyFrom(aggregator_audio_meta_info)


def _get_begin_stream(mime, tts_backend_response=None, aggregator_audio_meta_info=None):
    audio = TAudio(
        BeginStream=TBeginStream(
            Mime=mime,
        ),
    )

    _add_meta_info_to_audio(audio, tts_backend_response, aggregator_audio_meta_info)

    return audio


def _get_audio_chunk(audio, tts_backend_response=None, aggregator_audio_meta_info=None):
    audio = TAudio(
        Chunk=TAudioChunk(
            Data=audio,
        ),
    )

    _add_meta_info_to_audio(audio, tts_backend_response, aggregator_audio_meta_info)

    return audio


def _get_end_stream(tts_backend_response=None, aggregator_audio_meta_info=None):
    audio = TAudio(
        EndStream=TEndStream(),
    )

    _add_meta_info_to_audio(audio, tts_backend_response, aggregator_audio_meta_info)

    return audio


def _get_meta_info_only(tts_backend_response=None, aggregator_audio_meta_info=None):
    audio = TAudio(
        MetaInfoOnly=TMetaInfoOnly(),
    )

    _add_meta_info_to_audio(audio, tts_backend_response, aggregator_audio_meta_info)

    return audio


def _get_cache_get_response(key, status, cache_entry):
    return TCacheGetResponse(
        Key=key,
        Status=status,
        CacheEntry=cache_entry,
    )


def _get_cache_set_request(key, cache_entry):
    return TCacheSetRequest(
        Key=key,
        CacheEntry=cache_entry,
    )


def _get_cache_entry(audio, timings, duration):
    return TCacheEntry(
        Audio=audio,
        Timings=timings,
        Duration=duration,
    )


def _check_timings(timings, expected_timings):
    assert len(timings) == len(expected_timings)
    for i in range(len(expected_timings)):
        assert len(timings[i].timings) == len(expected_timings[i].timings)
        for j in range(len(expected_timings[i].timings)):
            assert abs(timings[i].timings[j].time - expected_timings[i].timings[j].time) < COMPARISON_EPS
            assert timings[i].timings[j].phoneme == expected_timings[i].timings[j].phoneme


def _check_response(
    response,
    expected_audio_items,
    expected_cache_set_request_cache_entries,
    expected_timings_items,
    expected_rts_timings=None,
):
    assert len(list(response.get_items())) == len(expected_audio_items) + len(
        expected_cache_set_request_cache_entries
    ) + len(expected_timings_items)

    audio_items = list(response.get_item_datas(item_type=ItemTypes.AUDIO, proto_type=TAudio))
    assert len(audio_items) == len(expected_audio_items)
    for i in range(len(expected_audio_items)):
        if expected_audio_items[i].HasField("BeginStream"):
            assert audio_items[i].HasField("BeginStream")
            assert audio_items[i].BeginStream.Mime == expected_audio_items[i].BeginStream.Mime
        elif expected_audio_items[i].HasField("EndStream"):
            assert audio_items[i].HasField("EndStream")
        elif expected_audio_items[i].HasField("Chunk"):
            assert audio_items[i].HasField("Chunk")
            assert audio_items[i].Chunk.Data == expected_audio_items[i].Chunk.Data
        else:
            assert False, f"Unknown audio type {expected_audio_items[i]}"

        if expected_audio_items[i].HasField("TtsBackendResponse"):
            assert audio_items[i].HasField("TtsBackendResponse")
            assert audio_items[i].TtsBackendResponse.ReqSeqNo == expected_audio_items[i].TtsBackendResponse.ReqSeqNo
        elif expected_audio_items[i].HasField("TtsAggregatorAudioMetaInfo"):
            assert audio_items[i].HasField("TtsAggregatorAudioMetaInfo")
            assert (
                audio_items[i].TtsAggregatorAudioMetaInfo.FirstChunkAudioSource
                == expected_audio_items[i].TtsAggregatorAudioMetaInfo.FirstChunkAudioSource
            )
            assert (
                audio_items[i].TtsAggregatorAudioMetaInfo.CurrentChunkAudioSource
                == expected_audio_items[i].TtsAggregatorAudioMetaInfo.CurrentChunkAudioSource
            )
        else:
            assert not audio_items[i].HasField("TtsBackendResponse") and not audio_items[i].HasField(
                "TtsAggregatorAudioMetaInfo"
            )

    cache_set_requests = list(
        response.get_item_datas(item_type=ItemTypes.TTS_CACHE_SET_REQUEST, proto_type=TCacheSetRequest)
    )
    assert len(cache_set_requests) == len(expected_cache_set_request_cache_entries)
    for cache_set_request in cache_set_requests:
        cache_key = cache_set_request.Key
        assert cache_key in expected_cache_set_request_cache_entries

        cache_entry = cache_set_request.CacheEntry
        expected_cache_entry = expected_cache_set_request_cache_entries[cache_key]
        assert cache_entry.Audio == expected_cache_entry.Audio
        assert abs(cache_entry.Duration - expected_cache_entry.Duration) < COMPARISON_EPS

        _check_timings(cache_entry.Timings, expected_cache_entry.Timings)

    timings_items = list(response.get_item_datas(item_type=ItemTypes.TTS_TIMINGS, proto_type=TTimings))
    assert len(timings_items) == len(expected_timings_items)
    for i in range(len(expected_timings_items)):
        assert timings_items[i].IsFromCache == expected_timings_items[i].IsFromCache

        _check_timings(timings_items[i].Timings, expected_timings_items[i].Timings)

    if expected_rts_timings:
        audio_chunk_num = 0
        for audio in audio_items:
            if audio.HasField("Chunk"):
                assert audio_chunk_num < len(expected_rts_timings)
                assert audio.Chunk.HasField("Timings")
                sum_frames_size = 0
                sum_frames_duration = 0
                durations = defaultdict(int)
                for r in audio.Chunk.Timings.Records:
                    # print("SIZE:{}, MILLISEC:{}".format(r.Size, r.Milliseconds))
                    sum_frames_size += r.Size
                    sum_frames_duration += r.Milliseconds
                    durations[r.Milliseconds] += 1
                # print("TOTAL_SIZE:{}, TOTAL_MILLISEC:{}".format(sum_frames_size, sum_frames_duration))
                assert sum_frames_size == expected_rts_timings[audio_chunk_num]['data_size']
                assert sum_frames_duration <= expected_rts_timings[audio_chunk_num]['max_audio_duration']
                assert sum_frames_duration >= expected_rts_timings[audio_chunk_num]['min_audio_duration']
                for chunk_duration, expected_min_chunks_count in expected_rts_timings[audio_chunk_num][
                    'min_chunks_with_duration'
                ].items():
                    assert durations[chunk_duration] >= expected_min_chunks_count
                audio_chunk_num += 1


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except TimeoutError:
        pass


# -------------------------------------------------------------------------------------------------
class TestTtsAggregator:
    @pytest.mark.asyncio
    @pytest.mark.parametrize('enable_save_to_cache', [False, True])
    async def test_simple(self, cuttlefish: Cuttlefish, enable_save_to_cache):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0)
                                        )
                                    ],
                                    "fake_cache_key",
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(2)], "cache_key2"
                                ),
                                # Yes, this is impossible (we must have fallback to real tts backend response)
                                # However we want to have as simple test as possible
                                _get_aggregator_audio_part(
                                    [_get_aggregator_cache_get_response_audio_source()], "cache_key_old"
                                ),
                                # Same cache key with other audio part
                                # only one of this items must be sent to tts cache
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(3)], "cache_key1"
                                ),
                                # Same key with other cache_get_response_audio_source
                                _get_aggregator_audio_part(
                                    [_get_aggregator_cache_get_response_audio_source()], "cache_key_old"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=True,
                            EnableSaveToCache=enable_save_to_cache,
                        ),
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream1_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            if enable_save_to_cache:
                expected_cache_set_request_cache_entries = {
                    "cache_key0": _get_cache_entry(b"tts_audio_stream0_chunk0", [], 0.0),
                }
            else:
                expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        _get_cache_get_response(
                            "cache_key_old",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(b"tts_audio_from_cache", [], 3.3),
                        )
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0): [
                        _get_http_response(b"s3_audio0")
                    ],
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                    timings=GenerateResponse.Timings(),
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=3),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream3_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=3,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=3),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_audio_chunk(
                    audio=b"s3_audio0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.S3_AUDIO,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream1_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream2_chunk0tts_audio_stream2_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_from_cache",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream3_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_from_cache",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            if enable_save_to_cache:
                expected_cache_set_request_cache_entries = {
                    "cache_key1": _get_cache_entry(b"tts_audio_stream1_chunk0", [], 0.0),
                    "cache_key2": _get_cache_entry(b"tts_audio_stream2_chunk0tts_audio_stream2_chunk1", [], 0.0),
                }
            else:
                expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    async def test_multisource(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                # For now there are no s3 audio in tts cache
                                # however this is just test, so we can do what ever we want
                                # Who knows, maybe we'll start writing s3 to the cache someday because it's faster
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0)
                                        ),
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 1)
                                        ),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key0",
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 2)
                                        ),
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 3)
                                        ),
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 4)
                                        ),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key1",
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 5)
                                        ),
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 6)
                                        ),
                                        _get_aggregator_http_response_audio_source(
                                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 7)
                                        ),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key2",
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_protocol_audio_audio_source(0),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key3",
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_protocol_audio_audio_source(1),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key4",
                                ),
                                _get_aggregator_audio_part(
                                    [
                                        _get_aggregator_protocol_audio_audio_source(2),
                                        _get_aggregator_cache_get_response_audio_source(),
                                    ],
                                    "cache_key5",
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=True,
                            EnableSaveToCache=False,
                        ),
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Bad responses
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0): [
                        _get_http_response(content=b"s3_audio_bad0", status_code=404)
                    ],
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 2): [
                        _get_http_response(content=b"s3_audio_bad2", status_code=503)
                    ],
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 5): [
                        _get_http_response(content=b"s3_audio_bad5", status_code=404)
                    ],
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        _get_cache_get_response(
                            "cache_key1",
                            ECacheGetResponseStatus.ERROR,
                            _get_cache_entry(b"s3_audio_from_cache_bad1", [], 0.0),
                        ),
                        _get_cache_get_response(
                            "cache_key2",
                            ECacheGetResponseStatus.MISS,
                            _get_cache_entry(b"s3_audio_from_cache_bad2", [], 0.0),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Bad and ok responses for third audio part in one chunk
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 6): [
                        _get_http_response(content=b"s3_audio_bad6", status_code=404)
                    ],
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 7): [
                        _get_http_response(content=b"s3_audio_ok7")
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Start audio stream for fourth part
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                ),
                            ),
                        ),
                    ],
                    # Ok response from cache for fifth audio part
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        _get_cache_get_response(
                            "cache_key4",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(b"tts_audio_from_cache_ok4", [], 0.0),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Already got bad response from this sources and do not track it
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0): [
                        _get_http_response(content=b"s3_audio_ok0")
                    ],
                    # Just ok response for second audio part
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 3): [
                        _get_http_response(content=b"s3_audio_ok3")
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Start audio stream for fifth part (must be ingnored because already got ok response from cache)
                    # Send failed tts backend response for sixth part
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream1_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_meta_info_only(
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=410,
                                ),
                            ),
                        ),
                    ],
                    # Ok response from cache for fourth audio part
                    # Must be ignored because audio stream already started
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        _get_cache_get_response(
                            "cache_key3",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(b"tts_audio_from_cache_ok3", [], 0.0),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    # Another ok response for second audio part
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 4): [
                        _get_http_response(content=b"s3_audio_ok4")
                    ],
                    # Finish stream for fourth audio part
                    ItemTypes.AUDIO: [
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                    ],
                    # Finally ok response for first and sixth audio parts
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        _get_cache_get_response(
                            "cache_key0",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(b"s3_audio_from_cache_ok0", [], 0.0),
                        ),
                        _get_cache_get_response(
                            "cache_key5",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(b"tts_audio_from_cache_ok5", [], 0.0),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"s3_audio_from_cache_ok0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"s3_audio_ok3",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.S3_AUDIO,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"s3_audio_ok7",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.S3_AUDIO,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0tts_audio_stream0_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_from_cache_ok4",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_from_cache_ok5",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('need_tts_backend_timings', [False, True])
    async def test_with_timings(self, cuttlefish: Cuttlefish, need_tts_backend_timings):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            PART_DURATIONS = [
                3.5,
                7.4,
                9.1,
                11.0,
            ]
            PART_DURATIONS_PREFIX = [PART_DURATIONS[0]]
            for chunk_duration in PART_DURATIONS[1:]:
                PART_DURATIONS_PREFIX.append(PART_DURATIONS_PREFIX[-1] + chunk_duration)

            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_cache_get_response_audio_source()], "cache_key_old"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(2)], "cache_key2"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=need_tts_backend_timings,
                            EnableSaveToCache=True,
                        ),
                    ],
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        # Second audio part
                        _get_cache_get_response(
                            "cache_key_old",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(
                                b"tts_audio_from_cache",
                                _get_tts_backend_timings_multi(3, 7),
                                PART_DURATIONS[1],
                            ),
                        )
                    ],
                    ItemTypes.AUDIO: [
                        # First audio part
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(4),
                                    duration=1.0,
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(4),
                                    duration=2.5,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        # Third audio part
                        # Check timings in all possible placess
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(11),
                                    duration=0.5,
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream1_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    timings=_get_tts_backend_timings(11),
                                    duration=1.0,
                                ),
                            ),
                        ),
                        _get_meta_info_only(
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    timings=_get_tts_backend_timings(11),
                                    duration=1.5,
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream1_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    timings=_get_tts_backend_timings(11),
                                    duration=2.5,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    timings=_get_tts_backend_timings(11),
                                    duration=3.6,
                                ),
                            ),
                        ),
                        # Fourth audio part
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(15),
                                    duration=PART_DURATIONS[3],
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0tts_audio_stream0_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_from_cache",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream1_chunk0tts_audio_stream1_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream2_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = {
                "cache_key0": _get_cache_entry(
                    b"tts_audio_stream0_chunk0tts_audio_stream0_chunk1",
                    _get_tts_backend_timings_multi(2, 4),
                    PART_DURATIONS[0],
                ),
                "cache_key1": _get_cache_entry(
                    b"tts_audio_stream1_chunk0tts_audio_stream1_chunk1",
                    _get_tts_backend_timings_multi(5, 11),
                    PART_DURATIONS[2],
                ),
                "cache_key2": _get_cache_entry(
                    b"tts_audio_stream2_chunk0", _get_tts_backend_timings_multi(1, 15), PART_DURATIONS[3]
                ),
            }

            if need_tts_backend_timings:
                expected_timings = [
                    TTimings(
                        Timings=_get_tts_backend_timings_multi(2, 4),
                        IsFromCache=False,
                    ),
                    TTimings(
                        Timings=_get_tts_backend_timings_multi(3, 7, time_shift=PART_DURATIONS_PREFIX[0] * 1000),
                        IsFromCache=True,
                    ),
                    TTimings(
                        Timings=_get_tts_backend_timings_multi(5, 11, time_shift=PART_DURATIONS_PREFIX[1] * 1000),
                        IsFromCache=False,
                    ),
                    TTimings(
                        Timings=_get_tts_backend_timings_multi(1, 15, time_shift=PART_DURATIONS_PREFIX[2] * 1000),
                        IsFromCache=False,
                    ),
                ]
            else:
                expected_timings = []

            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('enable_save_to_cache', [False, True])
    async def test_audio_corner_cases(self, cuttlefish: Cuttlefish, enable_save_to_cache):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(2)], "cache_key2"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(3)], "cache_key3"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=True,
                            EnableSaveToCache=enable_save_to_cache,
                        ),
                    ]
                }
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=3),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                ),
                            ),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream3_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=3,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream3_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=3,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                ),
                            ),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk1",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk2",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    timings=_get_tts_backend_timings(3),
                                ),
                            ),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_audio_chunk(
                    audio=b"tts_audio_stream2_chunk0tts_audio_stream2_chunk1tts_audio_stream2_chunk2",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            if enable_save_to_cache:
                expected_cache_set_request_cache_entries = {
                    "cache_key0": _get_cache_entry(b"tts_audio_stream0_chunk0tts_audio_stream0_chunk1", [], 0.0),
                    "cache_key1": _get_cache_entry(b"", [], 0.0),
                }
            else:
                expected_cache_set_request_cache_entries = dict()
            expected_timings = [
                TTimings(
                    Timings=_get_tts_backend_timings_multi(1, 3),
                    IsFromCache=False,
                ),
            ]
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=3),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk3",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    timings=_get_tts_backend_timings(3),
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        # This chunk after end stream must be ignored
                        _get_audio_chunk(
                            audio=b"tts_audio_stream2_chunk4",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                ),
                            ),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_audio_chunk(
                    audio=b"tts_audio_stream2_chunk3",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream3_chunk0tts_audio_stream3_chunk1",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            if enable_save_to_cache:
                expected_cache_set_request_cache_entries = {
                    "cache_key2": _get_cache_entry(
                        b"tts_audio_stream2_chunk0tts_audio_stream2_chunk1tts_audio_stream2_chunk2tts_audio_stream2_chunk3",
                        _get_tts_backend_timings_multi(2, 3),
                        0.0,
                    ),
                    "cache_key3": _get_cache_entry(
                        b"tts_audio_stream3_chunk0tts_audio_stream3_chunk1",
                        _get_tts_backend_timings_multi(2, 11),
                        0.0,
                    ),
                }
            else:
                expected_cache_set_request_cache_entries = dict()
            expected_timings = [
                TTimings(
                    Timings=_get_tts_backend_timings_multi(1, 3),
                    IsFromCache=False,
                ),
                TTimings(
                    Timings=_get_tts_backend_timings_multi(2, 11),
                    IsFromCache=False,
                ),
            ]
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    async def test_with_rts_opus_timings(self, cuttlefish: Cuttlefish):
        base64gzippedOpus = b'H4sICNGP5WEAA3plcm8ub3B1cwDzT08PZmBigALzc1dyQfQapi26jML+BaXFHqmJKYyMFowNdmAF/'
        base64gzippedOpus += b'iDlDMjKGYH0tKoVvcz///8DaQhJTC/mBYrlZCblA7kKhnqGekYgRapA7Orn7O/iGmQLkknNS1ZIK'
        base64gzippedOpus += b'8rPVQBxdEvy83OKFQyAii0ZRsEoGHIAkjMadkN4oJwBylVslv/PGzGTDHb8/zcIETTzlzPCvcgMp'
        base64gzippedOpus += b'D2jy0KGmRcbjJjgXmQB0r0NmfuHmRcZ3iG8yAqk3ZO3dQwzLzasZIZ7kQ1IK1j9+Tm8vMhicRDhR'
        base64gzippedOpus += b'XYgrX3qZD87PncDAG1B5UfvBwAA'
        gzippedOpus = base64.b64decode(base64gzippedOpus)
        opusData = zlib.decompress(gzippedOpus, 16 + zlib.MAX_WBITS)
        splitByFramePos = opusData.find(b'OggS', 0x500)
        opusData1from2 = opusData[:splitByFramePos]
        opusData2from2 = opusData[splitByFramePos:]
        opusData1from2bad = opusData[:0x500]
        opusData2from2bad = opusData[0x500:]

        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_cache_get_response_audio_source()], "cache_key_old"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(2)], "cache_key2"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedRtsTimings=True,
                            RtsBufferSeconds=4,
                            EnableSaveToCache=False,
                        ),
                    ],
                    ItemTypes.TTS_CACHE_GET_RESPONSE: [
                        # Second audio part
                        _get_cache_get_response(
                            "cache_key_old",
                            ECacheGetResponseStatus.HIT,
                            _get_cache_entry(
                                opusData,
                                _get_tts_backend_timings_multi(3, 7),
                                7.4,
                            ),
                        )
                    ],
                    ItemTypes.AUDIO: [
                        # First audio part
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=opusData1from2bad,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(4),
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=opusData2from2bad,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(4),
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        # Third audio part
                        # Check timings in all possible placess
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=opusData1from2,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        _get_meta_info_only(
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        _get_audio_chunk(
                            audio=opusData2from2,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    timings=_get_tts_backend_timings(11),
                                ),
                            ),
                        ),
                        # Fourth audio part
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                        _get_audio_chunk(
                            audio=opusData,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=2,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                    timings=_get_tts_backend_timings(15),
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=2),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=opusData,
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=opusData,
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_CACHE,
                    ),
                ),
                _get_audio_chunk(
                    audio=opusData,
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=opusData,
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = {}

            expected_timings = []

            expected_rts_timings = [  # sum chunks size, sum milliseconds in chunks
                {
                    'data_size': len(opusData),
                    'max_audio_duration': 1140,
                    'min_audio_duration': 1100,
                    'min_chunks_with_duration': {
                        250: 0,  # first chunk duration reduced by RtfBuffer value (4sec - see above ^^^)
                        20: 5,
                    },
                },
                {
                    'data_size': len(opusData),
                    'max_audio_duration': 5140,
                    'min_audio_duration': 5100,
                    'min_chunks_with_duration': {20: 255},
                },
                {
                    'data_size': len(opusData),
                    'max_audio_duration': 5140,
                    'min_audio_duration': 5100,
                    'min_chunks_with_duration': {20: 255},
                },
                {
                    'data_size': len(opusData),
                    'max_audio_duration': 5140,
                    'min_audio_duration': 5100,
                    'min_chunks_with_duration': {20: 255},
                },
            ]

            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
                expected_rts_timings,
            )

    @pytest.mark.asyncio
    async def test_empty_output(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=False,
                            EnableSaveToCache=False,
                        ),
                    ]
                }
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            "audio/opus",
                            _get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.NOT_SET,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    async def test_audio_source_error_after_start(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=False,
                            EnableSaveToCache=False,
                        ),
                    ]
                }
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=b"tts_audio_stream0_chunk0",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=False,
                                    response_code=200,
                                ),
                            ),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_audio_chunk(
                            audio=b"trash",
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=410,
                                    message="some error",
                                ),
                            ),
                        ),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)
            assert (
                response.has_exception()
                and b"Audio stream error with code: '410', message: 'some error'" in response.get_exception()
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('finish_and_error_in_same_chunk', [False, True])
    async def test_audio_source_error_after_finish(self, cuttlefish: Cuttlefish, finish_and_error_in_same_chunk):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=False,
                            EnableSaveToCache=False,
                        ),
                    ]
                }
            )

            audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0",
                    tts_backend_response=_get_tts_backend_response(
                        req_seq_no=0,
                        tts_generate_response=_get_tts_generate_response(
                            completed=False,
                            response_code=200,
                        ),
                    ),
                ),
                _get_end_stream(
                    tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                ),
            ]
            chunk_with_error = _get_audio_chunk(
                audio=b"trash",
                tts_backend_response=_get_tts_backend_response(
                    req_seq_no=0,
                    tts_generate_response=_get_tts_generate_response(
                        completed=True,
                        response_code=410,
                        message="some error",
                    ),
                ),
            )

            if finish_and_error_in_same_chunk:
                audio_items.append(chunk_with_error)

            stream.write_items(
                {
                    ItemTypes.AUDIO: audio_items,
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=b"tts_audio_stream0_chunk0",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            if not finish_and_error_in_same_chunk:
                stream.write_items(
                    {
                        ItemTypes.AUDIO: [chunk_with_error],
                    }
                )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

    @pytest.mark.asyncio
    async def test_with_background(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_AGGREGATOR) as stream:
            merger_test_data_path = yatest.common.work_path("test_data_for_merger")

            with open(f"{merger_test_data_path}/main_audio.opus", "rb") as f:
                # We will send this audio two times
                main_audio = f.read()

            with open(f"{merger_test_data_path}/background.pcm", "rb") as f:
                background_audio = f.read()

            output_chunks = []
            for i in range(3):
                with open(f"{merger_test_data_path}/output_chunk{i}", "rb") as f:
                    output_chunks.append(f.read())

            stream.write_items(
                {
                    ItemTypes.TTS_AGGREGATOR_REQUEST: [
                        TAggregatorRequest(
                            AudioParts=[
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(0)], "cache_key0"
                                ),
                                _get_aggregator_audio_part(
                                    [_get_aggregator_protocol_audio_audio_source(1)], "cache_key1"
                                ),
                            ],
                            Mime="audio/opus",
                            NeedTtsBackendTimings=False,
                            EnableSaveToCache=False,
                            BackgroundAudio=_get_aggregator_http_response_audio_source(
                                _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, "background")
                            ),
                        ),
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                        _get_audio_chunk(
                            audio=main_audio,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=0,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                ),
                            ),
                        ),
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=0),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts aggregator return something")

            stream.write_items(
                {
                    _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, "background"): [
                        _get_http_response(background_audio)
                    ],
                    ItemTypes.AUDIO: [
                        _get_begin_stream(
                            mime="audio/opus",
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                        _get_audio_chunk(
                            audio=main_audio,
                            tts_backend_response=_get_tts_backend_response(
                                req_seq_no=1,
                                tts_generate_response=_get_tts_generate_response(
                                    completed=True,
                                    response_code=200,
                                ),
                            ),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_begin_stream(
                    mime="audio/opus",
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        first_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.TTS_BACKEND,
                    ),
                ),
                _get_audio_chunk(
                    audio=output_chunks[0],
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.MERGED,
                    ),
                ),
                _get_audio_chunk(
                    audio=output_chunks[1],
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.MERGED,
                    ),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=5.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )

            stream.write_items(
                {
                    ItemTypes.AUDIO: [
                        _get_end_stream(
                            tts_backend_response=_get_tts_backend_response(req_seq_no=1),
                        ),
                    ],
                }
            )

            expected_audio_items = [
                _get_audio_chunk(
                    audio=output_chunks[2],
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(
                        current_chunk_audio_source=TAggregatorAudioMetaInfo.EAudioSource.MERGED,
                    ),
                ),
                _get_end_stream(
                    aggregator_audio_meta_info=_get_aggregator_audio_meta_info(),
                ),
            ]
            expected_cache_set_request_cache_entries = dict()
            expected_timings = []
            response = await stream.read(timeout=5.0)
            _check_response(
                response,
                expected_audio_items,
                expected_cache_set_request_cache_entries,
                expected_timings,
            )
