from .tts_cache_proxy import TtsCacheProxy

from alice.cuttlefish.library.protos.tts_pb2 import (
    TCacheGetRequest,
    TCacheSetRequest,
    TCacheWarmUpRequest,
    TCacheGetResponse,
    ECacheGetResponseStatus,
    TCacheGetResponseStatus,
    TCacheEntry,
)
from alice.cuttlefish.library.python.testing.constants import ItemTypes
from alice.cachalot.tests.integration_tests.lib import CachalotFixture as Cachalot

from alice.uniproxy.library.protos.ttscache_pb2 import TtsCacheRecord

from voicetech.library.proto_api.ttsbackend_pb2 import GenerateResponse

import asyncio

import pytest
import os
import time
import urllib


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cachalot():
    with Cachalot() as x:
        yield x


@pytest.fixture(scope="module")
def tts_cache_proxy(cachalot: Cachalot):
    with TtsCacheProxy(
        args=[
            "-V",
            "server.rtlog.file_stat_check_period=0.1s",
            "-V",
            f"tts_cache.cachalot.host={cachalot.host}",
            "-V",
            f"tts_cache.cachalot.port={cachalot.http_port}",
        ]
    ) as x:
        yield x


# -------------------------------------------------------------------------------------------------
COMPARISON_EPS = 10**-9


def _create_tts_timings(timings_cnt, subtimings_cnt):
    return [
        GenerateResponse.Timings(
            timings=[
                GenerateResponse.Timings.Timing(
                    time=500 * j,
                    phoneme=f"phoneme_{j}",
                )
                for j in range(subtimings_cnt)
            ]
        )
        for i in range(timings_cnt)
    ]


def _reformat_guid(guid):
    guid = guid.replace("-", "")
    return f"{guid[:8]}-{guid[8:12]}-{guid[12:16]}-{guid[16:20]}-{guid[20:]}"


def _get_cache_get_responses(response):
    return [
        item.data
        for item in response.get_items(item_type=ItemTypes.TTS_CACHE_GET_RESPONSE, proto_type=TCacheGetResponse)
    ]


def _get_cache_get_response_statuses(response):
    return [
        item.data
        for item in response.get_items(
            item_type=ItemTypes.TTS_CACHE_GET_RESPONSE_STATUS, proto_type=TCacheGetResponseStatus
        )
    ]


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except asyncio.exceptions.TimeoutError:
        pass


def _check_cache_entry(cache_entry, audio, duration, timings_cnt, subtimings_cnt):
    assert cache_entry.Audio == audio
    assert (cache_entry.Duration - duration) < COMPARISON_EPS

    timings = cache_entry.Timings
    assert len(timings) == timings_cnt
    for timing in timings:
        assert len(timing.timings) == subtimings_cnt
        for i in range(len(timing.timings)):
            assert abs(timing.timings[i].time - (500 * i)) < COMPARISON_EPS
            assert timing.timings[i].phoneme == f"phoneme_{i}"


def _check_cache_get_response(
    tts_cache_proxy_stream_response, key, status, audio=None, duration=None, timings_cnt=None, subtimings_cnt=None
):
    cache_get_responses = _get_cache_get_responses(tts_cache_proxy_stream_response)
    assert len(cache_get_responses) == 1
    cache_get_response = cache_get_responses[0]
    assert cache_get_response.Key == key
    assert cache_get_response.Status == status
    assert cache_get_response.ErrorMessage == ""
    if audio is not None:
        _check_cache_entry(cache_get_response.CacheEntry, audio, duration, timings_cnt, subtimings_cnt)
    else:
        assert not cache_get_response.HasField("CacheEntry")

    cache_get_response_statuses = _get_cache_get_response_statuses(tts_cache_proxy_stream_response)
    assert len(cache_get_response_statuses) == 1
    cache_get_response_status = cache_get_response_statuses[0]
    assert cache_get_response_status.Key == key
    assert cache_get_response_status.Status == status
    assert cache_get_response_status.ErrorMessage == ""


# -------------------------------------------------------------------------------------------------
class TestTtsCacheProxy:
    @pytest.mark.asyncio
    async def test_logs_rotation(self, tts_cache_proxy: TtsCacheProxy):
        async def make_request_and_check_logs(guid):
            response = await tts_cache_proxy.make_grpc_request(
                items={
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(
                            Key="random_key",
                        ),
                    ]
                },
                timeout=1.0,
                guid=guid,
            )

            _check_cache_get_response(response, "random_key", ECacheGetResponseStatus.MISS)

            eventlog = [event for event in tts_cache_proxy.get_eventlog(from_beginning=True)]
            tts_cache_callbacks_frames = 0
            for event in eventlog:
                if event["EventBody"]["Type"] == "TtsCacheCallbacksFrame":
                    assert event["EventBody"]["Fields"]["GUID"] == _reformat_guid(guid)
                    tts_cache_callbacks_frames += 1

            assert tts_cache_callbacks_frames == 1

            rtlog = [r for r in tts_cache_proxy.get_rtlog(from_beginning=True)]

            tts_cache_callbacks_rtlog = 0
            for rec in rtlog[:10]:
                print(f"REC: {rec}")
                msg = rec["EventBody"]["Fields"].get("Message", "")
                if msg.startswith("TtsCacheCallbacksFrame:"):
                    assert f"{_reformat_guid(guid)}" in msg
                    tts_cache_callbacks_rtlog += 1
            assert tts_cache_callbacks_rtlog == 1

        await make_request_and_check_logs("01234567-89abcdef-01234567-89abcdef")

        # rotate eventlog
        os.rename(tts_cache_proxy.eventlog_path, tts_cache_proxy.eventlog_path + ".1")
        os.rename(tts_cache_proxy.rtlog_path, tts_cache_proxy.rtlog_path + ".1")
        urllib.request.urlopen(f"http://{tts_cache_proxy.http_endpoint}/admin?action=reopenlog")
        time.sleep(0.2)  # double rtlog's check period to be sure in rotation

        await make_request_and_check_logs("aaaaaaaa-bbbbbbbb-cccccccc-dddddddd")

    @pytest.mark.asyncio
    async def test_simple(self, tts_cache_proxy: TtsCacheProxy):
        async with tts_cache_proxy.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_WARM_UP_REQUEST: [
                        TCacheWarmUpRequest(
                            Key="random_key",
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts cache proxy return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(
                            Key="some_key",
                        ),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)
            _check_cache_get_response(response, "some_key", ECacheGetResponseStatus.MISS)

            await _check_no_output(stream, "No response expected, but tts cache proxy return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_SET_REQUEST: [
                        TCacheSetRequest(
                            Key="other_key",
                            CacheEntry=TCacheEntry(
                                Audio=b"audio_data",
                                Timings=_create_tts_timings(2, 3),
                                Duration=0.5,
                            ),
                        ),
                    ],
                }
            )

            await _check_no_output(stream, "No response expected, but tts cache proxy return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(
                            Key="other_key",
                        ),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)
            _check_cache_get_response(
                response,
                "other_key",
                ECacheGetResponseStatus.HIT,
                audio=b"audio_data",
                duration=0.5,
                timings_cnt=2,
                subtimings_cnt=3,
            )

    @pytest.mark.asyncio
    async def test_write_from_python_uniproxy_and_get_from_tts_cache_proxy(
        self, tts_cache_proxy: TtsCacheProxy, cachalot: Cachalot
    ):
        key = "uniproxy_key"
        tts_cache_record = TtsCacheRecord(
            audio_data=b"audio_data",
            lookup_rate=1,
            timings=_create_tts_timings(5, 3),
            duration=1.5,
        )

        cachalot_client = cachalot.get_sync_client()

        # Create cache record how it does python uniporxy
        set_rsp = cachalot_client.cache_set(key, tts_cache_record.SerializeToString())
        assert set_rsp["Status"] == "CREATED"
        assert set_rsp["SetResp"]["Key"] == key

        async with tts_cache_proxy.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(Key=key),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)
            _check_cache_get_response(
                response,
                key,
                ECacheGetResponseStatus.HIT,
                audio=b"audio_data",
                duration=1.5,
                timings_cnt=5,
                subtimings_cnt=3,
            )

    @pytest.mark.asyncio
    async def test_write_from_tts_cache_proxy_and_get_from_python_uniproxy(
        self, tts_cache_proxy: TtsCacheProxy, cachalot: Cachalot
    ):
        key = "tts_cache_proxy_key"
        async with tts_cache_proxy.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_SET_REQUEST: [
                        TCacheSetRequest(
                            Key=key,
                            CacheEntry=TCacheEntry(
                                Audio=b"audio_data",
                                Timings=_create_tts_timings(4, 6),
                                Duration=2.5,
                            ),
                        ),
                    ],
                }
            )

            # tts_cache_proxy needs some time to send request to cachalot
            # In fact it's faster than one second but who wants test flaps?
            await asyncio.sleep(1.0)

        cachalot_client = cachalot.get_sync_client()

        # Get cache record how it does python uniproxy
        get_rsp = cachalot_client.cache_get(key)
        assert get_rsp["Status"] == "OK"
        assert get_rsp["GetResp"]["Key"] == key
        tts_cache_record = TtsCacheRecord()
        tts_cache_record.ParseFromString(get_rsp["GetResp"]["Data"])

        assert tts_cache_record.audio_data == b"audio_data"
        assert abs(tts_cache_record.lookup_rate - 0.0) < COMPARISON_EPS
        assert abs(tts_cache_record.duration - 2.5) < COMPARISON_EPS

        timings = tts_cache_record.timings
        assert len(timings) == 4
        for timing in timings:
            assert len(timing.timings) == 6
            for i in range(len(timing.timings)):
                assert abs(timing.timings[i].time - (500 * i)) < COMPARISON_EPS
                assert timing.timings[i].phoneme == f"phoneme_{i}"

    @pytest.mark.asyncio
    async def test_cancel(self, tts_cache_proxy: TtsCacheProxy):
        # Threre was a bug that crashes tts_cache_proxy
        # with high probability in this test

        REQUEST_COUNT = 1000
        async with tts_cache_proxy.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(
                            Key=f"random_key_{i}",
                        )
                        for i in range(REQUEST_COUNT)
                    ],
                }
            )

            stream.cancel()
            await asyncio.sleep(1.0)

        # Check that tts_cache_proxy still alive
        async with tts_cache_proxy.create_apphost_grpc_stream() as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_REQUEST: [
                        TCacheGetRequest(
                            Key="random_key",
                        ),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)
            _check_cache_get_response(response, "random_key", ECacheGetResponseStatus.MISS)
