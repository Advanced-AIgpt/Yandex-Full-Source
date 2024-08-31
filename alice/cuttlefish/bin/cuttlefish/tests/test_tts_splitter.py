import pytest
from asyncio.exceptions import TimeoutError
from .common import Cuttlefish
from .common.utils import find_header

from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.protos.session_pb2 import TAudioOptions, TVoiceOptions, TRequestContext, TSessionContext
from alice.cuttlefish.library.protos.tts_pb2 import (
    TRequest as TTtsRequest,
    TBackendRequest as TTtsBackendRequest,
    TRequestSenderRequest,
    TAggregatorRequest,
    TCacheGetRequest,
    TCacheWarmUpRequest,
)

from voicetech.library.proto_api.ttsbackend_pb2 import Generate
from voicetech.library.settings_manager.proto.settings_pb2 import TManagedSettings

from apphost.lib.proto_answers.http_pb2 import THttpRequest


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish(
        env={
            "AWS_ACCESS_KEY_ID": "aws_access_key",
            "AWS_SECRET_ACCESS_KEY": "aws_secret_access_key",
        },
    ) as x:
        yield x


# -------------------------------------------------------------------------------------------------
COMPARISON_EPS = 10**-9


def _get_item_type_with_id(prefix, id):
    return f"{prefix}{id}"


def _get_tts_backend_request(
    text, req_seq_no, mime="audio/opus", lang="ru", volume=1.0, speed=1.0, need_timings=False, surface=""
):
    return TTtsBackendRequest(
        Generate=Generate(
            mime=mime, text=text, lang=lang, volume=volume, speed=speed, need_timings=need_timings, surface=surface
        ),
        ReqSeqNo=req_seq_no,
    )


def _check_partial_response(
    response,
    expected_cache_warm_up_request_keys,
):
    assert len(list(response.get_items())) == len(expected_cache_warm_up_request_keys)

    cache_warm_up_requests = list(
        response.get_item_datas(item_type=ItemTypes.TTS_CACHE_WARM_UP_REQUEST, proto_type=TCacheWarmUpRequest)
    )
    assert len(cache_warm_up_requests) == len(expected_cache_warm_up_request_keys)
    for cache_warm_up_request in cache_warm_up_requests:
        assert cache_warm_up_request.Key in expected_cache_warm_up_request_keys


def _check_final_response(
    response,
    expected_s3_audio_http_requests,
    expected_tts_request_sender_parts,
    expected_tts_aggregator_audio_parts,
    expected_cache_get_request_keys,
    expected_background_audio_path_for_s3,
    need_tts_backend_timings,
    enable_save_to_cache,
    do_not_log_texts,
):
    assert (
        len(list(response.get_items()))
        == int(len(expected_tts_request_sender_parts) > 0)
        + len(expected_s3_audio_http_requests)
        + len(expected_cache_get_request_keys)
        + int(expected_background_audio_path_for_s3 is not None)
        + 1
    )

    for i in range(len(expected_s3_audio_http_requests)):
        current_item_type = _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, i)
        s3_audio_http_request = response.get_only_item_data(item_type=current_item_type, proto_type=THttpRequest)
        assert s3_audio_http_request.Method == THttpRequest.Get
        assert s3_audio_http_request.Path == expected_s3_audio_http_requests[i]["path"]

        if expected_s3_audio_http_requests[i]["has_s3_sign"]:
            assert len(s3_audio_http_request.Headers) == 4
            # Current time is a part of signature
            # So we can't just canonize values of some headers
            assert find_header(s3_audio_http_request, "host") == expected_s3_audio_http_requests[i]["host"]
            assert find_header(s3_audio_http_request, "x-amz-date") is not None
            assert find_header(s3_audio_http_request, "x-amz-content-sha256") == "UNSIGNED-PAYLOAD"
            assert "AWS4-HMAC-SHA256" in find_header(s3_audio_http_request, "authorization")
        else:
            assert len(s3_audio_http_request.Headers) == 1
            assert find_header(s3_audio_http_request, "host") == expected_s3_audio_http_requests[i]["host"]

        assert s3_audio_http_request.Content == b""

    if expected_background_audio_path_for_s3 is not None:
        current_item_type = _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, "background")
        s3_audio_http_request = response.get_only_item_data(item_type=current_item_type, proto_type=THttpRequest)
        assert s3_audio_http_request.Method == THttpRequest.Get
        assert s3_audio_http_request.Path == expected_background_audio_path_for_s3
        assert len(s3_audio_http_request.Headers) == 1
        assert s3_audio_http_request.Headers[0].Name == "host"
        assert s3_audio_http_request.Headers[0].Value == "tts-audio.s3.mds.yandex.net"
        assert s3_audio_http_request.Content == b""
    else:
        assert (
            len(
                list(
                    response.get_items(
                        item_type=_get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, "background"),
                        proto_type=THttpRequest,
                    )
                )
            )
            == 0
        )

    if len(expected_tts_request_sender_parts) > 0:
        tts_request_sender_request = response.get_only_item_data(
            item_type=ItemTypes.TTS_REQUEST_SENDER_REQUEST, proto_type=TRequestSenderRequest
        )
        assert tts_request_sender_request.DoNotLogTexts == do_not_log_texts
        assert len(tts_request_sender_request.AudioPartGenerateRequests) == len(expected_tts_request_sender_parts)
        for i in range(len(expected_tts_request_sender_parts)):
            source_type, item_type, expected_request = expected_tts_request_sender_parts[i]
            assert tts_request_sender_request.AudioPartGenerateRequests[i].Request.ItemType == item_type

            if source_type == "tts_backend":
                tts_backend_request = tts_request_sender_request.AudioPartGenerateRequests[i].Request.BackendRequest
                # Compare only subset of important fields to reduce test size and make code more readable
                # Correct request content generation tested in other place (splitter/utils unittest)

                assert tts_backend_request.ReqSeqNo == expected_request.ReqSeqNo
                assert tts_backend_request.DoNotLogTexts == do_not_log_texts

                generate_request = tts_backend_request.Generate
                expected_generate_request = expected_request.Generate
                assert generate_request.text == expected_generate_request.text
                assert generate_request.lang == expected_generate_request.lang
                assert generate_request.mime == expected_generate_request.mime
                assert abs(generate_request.volume - expected_generate_request.volume) < COMPARISON_EPS
                assert abs(generate_request.speed - expected_generate_request.speed) < COMPARISON_EPS
                assert generate_request.need_timings == expected_generate_request.need_timings
                assert generate_request.do_not_log == do_not_log_texts
                assert generate_request.surface == expected_generate_request.surface
            else:
                assert False, f"Unknown source type {source_type}"
    else:
        assert (
            len(
                list(
                    response.get_items(item_type=ItemTypes.TTS_REQUEST_SENDER_REQUEST, proto_type=TRequestSenderRequest)
                )
            )
            == 0
        )

    tts_aggregator_request = response.get_only_item_data(
        item_type=ItemTypes.TTS_AGGREGATOR_REQUEST, proto_type=TAggregatorRequest
    )
    assert tts_aggregator_request.Mime == "audio/opus"
    assert tts_aggregator_request.NeedTtsBackendTimings == need_tts_backend_timings
    assert tts_aggregator_request.EnableSaveToCache == enable_save_to_cache
    assert tts_aggregator_request.DoNotLogTexts == do_not_log_texts
    if expected_background_audio_path_for_s3 is not None:
        assert tts_aggregator_request.BackgroundAudio.HttpResponse.ItemType == _get_item_type_with_id(
            ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, "background"
        )
    else:
        assert not tts_aggregator_request.HasField("BackgroundAudio")
    assert len(tts_aggregator_request.AudioParts) == len(expected_tts_aggregator_audio_parts)
    for i in range(len(expected_tts_aggregator_audio_parts)):
        assert tts_aggregator_request.AudioParts[i].CacheKey != ""

        for j in range(len(expected_tts_aggregator_audio_parts[i])):
            source_type, data = expected_tts_aggregator_audio_parts[i][j]

            if source_type == "tts_backend":
                assert tts_aggregator_request.AudioParts[i].AudioSources[j].Audio.ReqSeqNo == data
            elif source_type == "s3_audio":
                assert tts_aggregator_request.AudioParts[i].AudioSources[j].HttpResponse.ItemType == data
            elif source_type == "tts_cache":
                assert tts_aggregator_request.AudioParts[i].AudioSources[j].HasField("CacheGetResponse")
                assert tts_aggregator_request.AudioParts[i].CacheKey == data
            else:
                assert False, f"Unknown source type {source_type}"

    cache_get_requests = list(
        response.get_item_datas(item_type=ItemTypes.TTS_CACHE_GET_REQUEST, proto_type=TCacheGetRequest)
    )
    assert len(cache_get_requests) == len(expected_cache_get_request_keys)
    for cache_get_request in cache_get_requests:
        assert cache_get_request.Key in expected_cache_get_request_keys


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except TimeoutError:
        pass


# -------------------------------------------------------------------------------------------------
class TestTtsSplitter:
    @pytest.mark.asyncio
    # Every of this params change tts splitter response a little bit
    # so it's better to parametrize all than copypaste
    @pytest.mark.parametrize('replace_shitova_with_shitova_gpu', [False, True])
    @pytest.mark.parametrize('need_tts_backend_timings', [False, True])
    @pytest.mark.parametrize('enable_tts_backend_timings', [False, True])
    # In this test all partial request in same chunk with final
    # so no cache warm up requests expected in all cases
    @pytest.mark.parametrize('enable_cache_warm_up', [False, True])
    @pytest.mark.parametrize('enable_save_to_cache', [False, True])
    @pytest.mark.parametrize('do_not_log_texts', [False, True])
    async def test_no_get_from_cache(
        self,
        cuttlefish: Cuttlefish,
        replace_shitova_with_shitova_gpu,
        need_tts_backend_timings,
        enable_tts_backend_timings,
        enable_cache_warm_up,
        enable_save_to_cache,
        do_not_log_texts,
    ):
        shitova_voice_item_suffix = "ru_gpu_shitova.gpu" if replace_shitova_with_shitova_gpu else "ru_cpu_shitova"
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                            VoiceOptions=TVoiceOptions(
                                Quality=TVoiceOptions.HIGH,
                            ),
                        ),
                    ],
                    ItemTypes.SESSION_CONTEXT: [TSessionContext(Surface="navigator")],
                    ItemTypes.TTS_PARTIAL_REQUEST: [
                        TTtsRequest(
                            Text="some text",
                            PartialNumber=0,
                            ReplaceShitovaWithShitovaGpu=replace_shitova_with_shitova_gpu,
                            NeedTtsBackendTimings=need_tts_backend_timings,
                            EnableTtsBackendTimings=enable_tts_backend_timings,
                            EnableGetFromCache=False,
                            EnableCacheWarmUp=enable_cache_warm_up,
                            EnableSaveToCache=enable_save_to_cache,
                            DoNotLogTexts=do_not_log_texts,
                        ),
                    ],
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="""
                            Привет
                            <speaker volume='15.0' voice='oksana'>Громко
                            <speaker audio='file.opus' voice='shitova' volume='1.0'>Аудио и текст
                            <speaker voice='zahar.gpu'>А теперь Захар
                            <speaker voice='oksana' lang='en'>And now english
                            <speaker audio="file2.opus">
                            <speaker voice='sasha.gpu'>И ещё Саша
                            <speaker audio='file3.opus' s3_audio_bucket='alice-time-capsule'>
                            <speaker audio='file4.opus' s3_audio_bucket='tts-audio'>
                        """,
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=replace_shitova_with_shitova_gpu,
                            NeedTtsBackendTimings=need_tts_backend_timings,
                            EnableTtsBackendTimings=enable_tts_backend_timings,
                            EnableGetFromCache=False,
                            EnableCacheWarmUp=enable_cache_warm_up,
                            EnableSaveToCache=enable_save_to_cache,
                            DoNotLogTexts=do_not_log_texts,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = [
                {"path": "/file.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False},
                {"path": "/file2.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False},
                {"path": "/file3.opus", "host": "alice-time-capsule.s3.mds.yandex.net", "has_s3_sign": True},
                {"path": "/file4.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False},
            ]
            expected_tts_request_sender_parts = [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, shitova_voice_item_suffix),
                    _get_tts_backend_request("Привет", 0, need_timings=enable_tts_backend_timings, surface="navigator"),
                ),
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_cpu_oksana"),
                    _get_tts_backend_request(
                        "Громко", 1, volume=15.0, need_timings=enable_tts_backend_timings, surface="navigator"
                    ),
                ),
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, shitova_voice_item_suffix),
                    _get_tts_backend_request(
                        "Аудио и текст", 2, need_timings=enable_tts_backend_timings, surface="navigator"
                    ),
                ),
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_oksana_zahar.gpu"),
                    _get_tts_backend_request(
                        "А теперь Захар", 3, need_timings=enable_tts_backend_timings, surface="navigator"
                    ),
                ),
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "en_cpu_oksana"),
                    _get_tts_backend_request(
                        "And now english", 4, lang="en", need_timings=enable_tts_backend_timings, surface="navigator"
                    ),
                ),
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_oksana_sasha.gpu"),
                    _get_tts_backend_request(
                        "И ещё Саша", 5, need_timings=enable_tts_backend_timings, surface="navigator"
                    ),
                ),
            ]
            expected_tts_aggregator_audio_parts = [
                [("tts_backend", 0)],
                [("tts_backend", 1)],
                [("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0))],
                [("tts_backend", 2)],
                [("tts_backend", 3)],
                [("tts_backend", 4)],
                [("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 1))],
                [("tts_backend", 5)],
                [("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 2))],
                [("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 3))],
            ]
            expected_cache_get_request_keys = []

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=None,
                need_tts_backend_timings=need_tts_backend_timings,
                enable_save_to_cache=(enable_save_to_cache and not do_not_log_texts),
                do_not_log_texts=do_not_log_texts,
            )

    @pytest.mark.asyncio
    async def test_with_get_from_cache(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                            VoiceOptions=TVoiceOptions(
                                Quality=TVoiceOptions.HIGH,
                            ),
                        ),
                    ],
                    # 2 test0
                    # 1 test1
                    # 1 test2
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="<speaker>test0<speaker audio='fake.opus'><speaker>test1<speaker>test0<speaker voice='zahar.gpu'>test2",
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableGetFromCache=True,
                            EnableCacheWarmUp=False,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = [
                {"path": "/fake.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False}
            ]
            expected_tts_request_sender_parts = [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_shitova.gpu"),
                    _get_tts_backend_request(f"test{test_id}", req_seq_no),
                )
                for req_seq_no, test_id in enumerate((0, 1, 0))
            ] + [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_oksana_zahar.gpu"),
                    _get_tts_backend_request("test2", 3),
                ),
            ]
            expected_tts_aggregator_audio_parts = [
                [
                    ("tts_backend", 0),
                    ("tts_cache", "trunk_tts_cache_276e60429872604450adc4a33f34d62f_0ef08c830ca1a266efe2829fcc5df9bf"),
                ],
                [
                    ("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0)),
                ],
                [
                    ("tts_backend", 1),
                    ("tts_cache", "trunk_tts_cache_41c71d1ef2460a191eb730b8a4bbccfa_0ef08c830ca1a266efe2829fcc5df9bf"),
                ],
                [
                    ("tts_backend", 2),
                    ("tts_cache", "trunk_tts_cache_276e60429872604450adc4a33f34d62f_0ef08c830ca1a266efe2829fcc5df9bf"),
                ],
                [
                    ("tts_backend", 3),
                    ("tts_cache", "trunk_tts_cache_8c342aa38294d8f0674916cb6fef479b_1f2b298a7f68e375f7abddd62130d47b"),
                ],
            ]
            expected_cache_get_request_keys = [
                "trunk_tts_cache_276e60429872604450adc4a33f34d62f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_41c71d1ef2460a191eb730b8a4bbccfa_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_8c342aa38294d8f0674916cb6fef479b_1f2b298a7f68e375f7abddd62130d47b",
            ]

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=None,
                need_tts_backend_timings=False,
                enable_save_to_cache=False,
                do_not_log_texts=False,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('enable_cache_warm_up', [False, True])
    async def test_cache_warm_up(self, cuttlefish: Cuttlefish, enable_cache_warm_up):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                        ),
                    ],
                    # 3 test0, 1 test1, 2 test2
                    # We must warm up every item only once
                    ItemTypes.TTS_PARTIAL_REQUEST: [
                        TTtsRequest(
                            Text="<speaker>test0<speaker audio=\"fake0.opus\"<speaker>test1<speaker>test0",
                            PartialNumber=0,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=enable_cache_warm_up,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                        TTtsRequest(
                            Text="<speaker>test0<speaker audio=\"fake1.opus\"<speaker>test2<speaker>test2",
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=enable_cache_warm_up,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            if enable_cache_warm_up:
                expected_cache_warm_up_request_keys = {
                    "trunk_tts_cache_276e60429872604450adc4a33f34d62f_0ef08c830ca1a266efe2829fcc5df9bf",
                    "trunk_tts_cache_41c71d1ef2460a191eb730b8a4bbccfa_0ef08c830ca1a266efe2829fcc5df9bf",
                    "trunk_tts_cache_8c342aa38294d8f0674916cb6fef479b_0ef08c830ca1a266efe2829fcc5df9bf",
                }

                response = await stream.read(timeout=1.0)
                _check_partial_response(response, expected_cache_warm_up_request_keys)

            await _check_no_output(stream, "No response expected, but tts splitter return something")

            stream.write_items(
                {
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="final",
                            PartialNumber=2,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=enable_cache_warm_up,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = []
            expected_tts_request_sender_parts = [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_shitova.gpu"),
                    _get_tts_backend_request("final", 0),
                ),
            ]
            expected_tts_aggregator_audio_parts = [
                [("tts_backend", 0)],
            ]
            expected_cache_get_request_keys = []

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=None,
                need_tts_backend_timings=False,
                enable_save_to_cache=False,
                do_not_log_texts=False,
            )

    @pytest.mark.asyncio
    async def test_final_request_items_limit_exceeded(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            DEFAULT_ITEMS_LIMIT = 32

            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                        ),
                    ],
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="".join(
                                [
                                    f"<speaker audio=\"fake{i}.opus\"<speaker voice='zahar.gpu'>test_backend{i}"
                                    for i in range(DEFAULT_ITEMS_LIMIT + 200)
                                ]
                            ),
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=False,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = [
                {"path": f"/fake{i}.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False}
                for i in range(DEFAULT_ITEMS_LIMIT)
            ]

            expected_tts_request_sender_parts = [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_oksana_zahar.gpu"),
                    _get_tts_backend_request(f"test_backend{i}", i),
                )
                for i in range(DEFAULT_ITEMS_LIMIT)
            ]
            expected_tts_aggregator_audio_parts = [
                [
                    (
                        ("s3_audio", "tts_backend")[i % 2],
                        (
                            _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, i // 2),
                            i // 2,
                        )[i % 2],
                    )
                ]
                for i in range(2 * DEFAULT_ITEMS_LIMIT)
            ]
            expected_cache_get_request_keys = []

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=None,
                need_tts_backend_timings=False,
                enable_save_to_cache=False,
                do_not_log_texts=False,
            )

    @pytest.mark.asyncio
    async def test_partial_request_items_limit_exceeded(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            DEFAULT_ITEMS_LIMIT = 32

            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                        ),
                    ],
                    ItemTypes.TTS_PARTIAL_REQUEST: [
                        TTtsRequest(
                            Text="".join(
                                [f"<speaker voice='shitova'>test{i}" for i in range(DEFAULT_ITEMS_LIMIT + 200)]
                            ),
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=True,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_cache_warm_up_request_keys = {
                "trunk_tts_cache_276e60429872604450adc4a33f34d62f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_41c71d1ef2460a191eb730b8a4bbccfa_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_8c342aa38294d8f0674916cb6fef479b_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_9587036d6ca7aa9baff1fd5c5204b789_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_6435d0fe9af62924fcef209f0d3d78b5_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_91b6b07b4b7b2598427a6924eda3bd9e_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_dbc3216496a3c03608898b56b8d5b822_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_115e75e67d0e470ed9912b2d98f441dd_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_217af6e30960544fd33b0280f171afba_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_eede70b3b48e589cb5a25ac966a0283b_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_fb3a0a428e89667aba13253e2123da14_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_e6676ebca0a354ab0a3e754233d690d2_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_54db2ca596a0048b92c9086a69a20a03_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_8773f2d41e59a74f6417cfe8abcea58f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_cedd8aee36cfeab217147d1e7e739c7f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_7c52eab742f70f7f8340e660ad184a72_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_9aa1a31f58e8586b4f2f17d8802a6f86_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_62ffb4732040f16f6774ce20361da56f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_3341ffbd62598230094941bc858c85b3_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_803527110fccd2575190dd556a6cda63_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_f9f60376558462d21f533651528dd65d_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_e37c6ad3a92a91d3c7ef91645f532b45_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_eae55ce053e15047f08e533b7581ebe0_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_116929dc2d97bfacd31393f5b2f16e9f_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_c6ffd941dd9387c05716141125bebc08_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_f903c3fcf426544e2780c00499309725_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_0b5727791c2a169e8b73442a17f9cbcd_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_11f4796b8ac2eff35909a9a064ceda50_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_0464afbecfb793073ab1ffd67fa62ae7_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_6896f5895a5320d67599a7356506befd_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_8f71812e888a2cbd07beb9b455aff7b4_0ef08c830ca1a266efe2829fcc5df9bf",
                "trunk_tts_cache_008c9893dc73ebf5e2ba7df1b69ce062_0ef08c830ca1a266efe2829fcc5df9bf",
            }

            response = await stream.read(timeout=1.0)
            _check_partial_response(response, expected_cache_warm_up_request_keys)

    @pytest.mark.asyncio
    async def test_no_tts_backend_requests(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                            VoiceOptions=TVoiceOptions(
                                Quality=TVoiceOptions.HIGH,
                            ),
                        ),
                    ],
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="<speaker audio='file.opus'>",
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=False,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = [
                {"path": "/file.opus", "host": "tts-audio.s3.mds.yandex.net", "has_s3_sign": False}
            ]
            expected_tts_request_sender_parts = []
            expected_tts_aggregator_audio_parts = [
                [("s3_audio", _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_RESPONSE_PREFIX, 0))],
            ]
            expected_cache_get_request_keys = []

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=None,
                need_tts_backend_timings=False,
                enable_save_to_cache=False,
                do_not_log_texts=False,
            )

    @pytest.mark.asyncio
    @pytest.mark.parametrize('enable_background_audio', [False, True])
    async def test_background_audio_request(self, cuttlefish: Cuttlefish, enable_background_audio):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                            VoiceOptions=TVoiceOptions(
                                Quality=TVoiceOptions.HIGH,
                            ),
                            SettingsFromManager=TManagedSettings(
                                TtsBackgroundAudio=enable_background_audio,
                            ),
                        ),
                    ],
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="<speaker background='test.pcm'>Привет",
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableGetFromCache=False,
                            EnableCacheWarmUp=False,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            expected_s3_audio_http_requests = []
            expected_tts_request_sender_parts = [
                (
                    "tts_backend",
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, "ru_gpu_shitova.gpu"),
                    _get_tts_backend_request("Привет", 0),
                ),
            ]
            expected_tts_aggregator_audio_parts = [
                [("tts_backend", 0)],
            ]
            expected_cache_get_request_keys = []

            response = await stream.read(timeout=1.0)
            _check_final_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_request_sender_parts,
                expected_tts_aggregator_audio_parts,
                expected_cache_get_request_keys,
                expected_background_audio_path_for_s3=("/test.pcm" if enable_background_audio else None),
                need_tts_backend_timings=False,
                enable_save_to_cache=False,
                do_not_log_texts=False,
            )

    @pytest.mark.asyncio
    async def test_not_allowed_s3_bucket(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_SPLITTER) as stream:
            stream.write_items(
                {
                    ItemTypes.REQUEST_CONTEXT: [
                        TRequestContext(
                            AudioOptions=TAudioOptions(
                                Format="audio/opus",
                            ),
                            VoiceOptions=TVoiceOptions(
                                Quality=TVoiceOptions.HIGH,
                            ),
                        ),
                    ],
                    ItemTypes.TTS_REQUEST: [
                        TTtsRequest(
                            Text="<speaker audio='file4.opus' s3_audio_bucket='not-allowed-bucket'>",
                            PartialNumber=1,
                            RequestId="some_request_id",
                            ReplaceShitovaWithShitovaGpu=True,
                            NeedTtsBackendTimings=False,
                            EnableTtsBackendTimings=False,
                            EnableCacheWarmUp=False,
                            EnableSaveToCache=False,
                            DoNotLogTexts=False,
                        ),
                    ],
                }
            )

            response = await stream.read(timeout=1.0)

            assert (
                response.has_exception()
                and b"S3 bucket 'not-allowed-bucket' is not allowed" in response.get_exception()
            )
