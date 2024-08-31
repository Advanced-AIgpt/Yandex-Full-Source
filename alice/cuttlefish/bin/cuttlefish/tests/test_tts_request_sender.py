import pytest
from asyncio.exceptions import TimeoutError
from .common import Cuttlefish

from alice.cuttlefish.library.python.testing.constants import ItemTypes, ServiceHandles
from alice.cuttlefish.library.protos.tts_pb2 import (
    TBackendRequest,
    TRequestSenderRequest,
    ECacheGetResponseStatus,
    TCacheGetResponseStatus,
)

from voicetech.library.proto_api.ttsbackend_pb2 import Generate

from apphost.lib.proto_answers.http_pb2 import THttpRequest


# -------------------------------------------------------------------------------------------------
@pytest.fixture(scope="module")
def cuttlefish():
    with Cuttlefish() as x:
        yield x


# -------------------------------------------------------------------------------------------------
def _get_item_type_with_id(prefix, id):
    return f"{prefix}{id}"


def _get_http_request(path):
    return THttpRequest(
        Method=THttpRequest.Get,
        Path=path,
        Content=b"",
    )


def _get_tts_backend_request(req_seq_no=227):
    return TBackendRequest(
        Generate=Generate(
            mime="audio/opus",
            text="test",
            lang="ru",
        ),
        ReqSeqNo=req_seq_no,
    )


def _get_rs_always_send_condition():
    return TRequestSenderRequest.TAudioPartGenerateRequest.TSendCondition(
        AlwaysSend=TRequestSenderRequest.TAudioPartGenerateRequest.TSendCondition.TAlwaysSend(),
    )


def _get_rs_cache_miss_or_error_condition(key):
    return TRequestSenderRequest.TAudioPartGenerateRequest.TSendCondition(
        CacheMissOrError=TRequestSenderRequest.TAudioPartGenerateRequest.TSendCondition.TCacheMissOrError(
            Key=key,
        ),
    )


def _get_rs_audio_part_generate_request_action(item_type, http_request=None, tts_backend_request=None):
    assert http_request is None or tts_backend_request is None, "Both http_request and tts_backend_request provided"

    if http_request is not None:
        return TRequestSenderRequest.TAudioPartGenerateRequest.TRequest(
            ItemType=item_type,
            HttpRequest=http_request,
        )
    elif tts_backend_request is not None:
        return TRequestSenderRequest.TAudioPartGenerateRequest.TRequest(
            ItemType=item_type,
            BackendRequest=tts_backend_request,
        )
    else:
        assert False, "Both http_request and tts_backend_request is None"


def _get_rs_audio_part_generate_request(send_condition, request):
    return TRequestSenderRequest.TAudioPartGenerateRequest(
        SendCondition=send_condition,
        Request=request,
    )


def _get_cache_get_response_status(key, status):
    return TCacheGetResponseStatus(
        Key=key,
        Status=status,
    )


def _check_response(
    response,
    expected_s3_audio_http_requests,
    expected_tts_backend_requests,
):
    assert len(list(response.get_items())) == len(expected_s3_audio_http_requests) + sum(
        [len(req_seq_nums) for item_type, req_seq_nums in expected_tts_backend_requests]
    )

    for id, path in expected_s3_audio_http_requests:
        current_item_type = _get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, id)
        s3_audio_http_request = response.get_only_item_data(item_type=current_item_type, proto_type=THttpRequest)
        assert s3_audio_http_request.Method == THttpRequest.Get
        assert s3_audio_http_request.Path == path
        assert s3_audio_http_request.Content == b""

    for item_type, req_seq_nums in expected_tts_backend_requests:
        tts_backend_requests = list(response.get_item_datas(item_type=item_type, proto_type=TBackendRequest))
        assert len(tts_backend_requests) == len(req_seq_nums)

        for i in range(len(req_seq_nums)):
            tts_backend_request = tts_backend_requests[i]
            assert tts_backend_request.ReqSeqNo == req_seq_nums[i]

            assert tts_backend_request.Generate.text == "test"
            assert tts_backend_request.Generate.lang == "ru"


async def _check_no_output(stream, error_message):
    try:
        await stream.read(timeout=1.0)
        assert False, error_message
    except TimeoutError:
        pass


# -------------------------------------------------------------------------------------------------
class TestTtsRequestSender:
    @pytest.mark.asyncio
    async def test_always_send(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_REQUEST_SENDER) as stream:
            S3_AUDIO_HTTP_COUNT = 3
            TTS_BACKEND_REQ_DEFAULT_COUNT = 4
            TTS_BACKEND_REQ_SPECIAL_COUNT = 5

            stream.write_items(
                {
                    ItemTypes.TTS_REQUEST_SENDER_REQUEST: [
                        TRequestSenderRequest(
                            AudioPartGenerateRequests=[
                                _get_rs_audio_part_generate_request(
                                    _get_rs_always_send_condition(),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=_get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, i),
                                        http_request=_get_http_request(f"/some_path?param={i}"),
                                    ),
                                )
                                for i in range(S3_AUDIO_HTTP_COUNT)
                            ]
                            + [
                                _get_rs_audio_part_generate_request(
                                    _get_rs_always_send_condition(),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=ItemTypes.TTS_BACKEND_REQUEST,
                                        tts_backend_request=_get_tts_backend_request(i),
                                    ),
                                )
                                for i in range(TTS_BACKEND_REQ_DEFAULT_COUNT)
                            ]
                            + [
                                _get_rs_audio_part_generate_request(
                                    _get_rs_always_send_condition(),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=_get_item_type_with_id(
                                            ItemTypes.TTS_BACKEND_REQUEST_PREFIX, i + TTS_BACKEND_REQ_DEFAULT_COUNT
                                        ),
                                        tts_backend_request=_get_tts_backend_request(i + TTS_BACKEND_REQ_DEFAULT_COUNT),
                                    ),
                                )
                                for i in range(TTS_BACKEND_REQ_SPECIAL_COUNT)
                            ]
                        ),
                    ]
                }
            )

            expected_s3_audio_http_requests = [(i, f"/some_path?param={i}") for i in range(S3_AUDIO_HTTP_COUNT)]
            expected_tts_backend_requests = [
                (ItemTypes.TTS_BACKEND_REQUEST, [i for i in range(TTS_BACKEND_REQ_DEFAULT_COUNT)])
            ] + [
                (
                    _get_item_type_with_id(ItemTypes.TTS_BACKEND_REQUEST_PREFIX, i + TTS_BACKEND_REQ_DEFAULT_COUNT),
                    [i + TTS_BACKEND_REQ_DEFAULT_COUNT],
                )
                for i in range(TTS_BACKEND_REQ_SPECIAL_COUNT)
            ]
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_backend_requests,
            )

    @pytest.mark.asyncio
    async def test_cache_miss_or_error(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_REQUEST_SENDER) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_REQUEST_SENDER_REQUEST: [
                        TRequestSenderRequest(
                            AudioPartGenerateRequests=[
                                _get_rs_audio_part_generate_request(
                                    _get_rs_cache_miss_or_error_condition(f"key_{i}"),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=_get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, i),
                                        http_request=_get_http_request(f"/some_path?param={i}"),
                                    ),
                                )
                                for i in range(6)
                            ]
                        ),
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts request sender return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_RESPONSE_STATUS: [
                        _get_cache_get_response_status("key_0", ECacheGetResponseStatus.HIT),
                        _get_cache_get_response_status("key_1", ECacheGetResponseStatus.MISS),
                        _get_cache_get_response_status("key_2", ECacheGetResponseStatus.HIT),
                        _get_cache_get_response_status("key_3", ECacheGetResponseStatus.ERROR),
                    ]
                }
            )

            expected_s3_audio_http_requests = [(i, f"/some_path?param={i}") for i in (1, 3)]
            expected_tts_backend_requests = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_backend_requests,
            )

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_RESPONSE_STATUS: [
                        _get_cache_get_response_status("key_5", ECacheGetResponseStatus.HIT),
                    ]
                }
            )

            await _check_no_output(stream, "No response expected, but tts request sender return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_RESPONSE_STATUS: [
                        _get_cache_get_response_status("key_4", ECacheGetResponseStatus.MISS),
                    ]
                }
            )

            expected_s3_audio_http_requests = [(i, f"/some_path?param={i}") for i in (4,)]
            expected_tts_backend_requests = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_backend_requests,
            )

    @pytest.mark.asyncio
    async def test_combined_conditions(self, cuttlefish: Cuttlefish):
        async with cuttlefish.create_apphost_grpc_stream(handle=ServiceHandles.TTS_REQUEST_SENDER) as stream:
            stream.write_items(
                {
                    ItemTypes.TTS_REQUEST_SENDER_REQUEST: [
                        TRequestSenderRequest(
                            AudioPartGenerateRequests=[
                                _get_rs_audio_part_generate_request(
                                    _get_rs_always_send_condition(),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=_get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, 0),
                                        http_request=_get_http_request("/some_path?param=0"),
                                    ),
                                ),
                                _get_rs_audio_part_generate_request(
                                    _get_rs_cache_miss_or_error_condition("key_1"),
                                    _get_rs_audio_part_generate_request_action(
                                        item_type=_get_item_type_with_id(ItemTypes.S3_AUDIO_HTTP_REQUEST_PREFIX, 1),
                                        http_request=_get_http_request("/some_path?param=1"),
                                    ),
                                ),
                            ]
                        ),
                    ]
                }
            )

            expected_s3_audio_http_requests = [(i, f"/some_path?param={i}") for i in (0,)]
            expected_tts_backend_requests = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_backend_requests,
            )

            await _check_no_output(stream, "No response expected, but tts request sender return something")

            stream.write_items(
                {
                    ItemTypes.TTS_CACHE_GET_RESPONSE_STATUS: [
                        _get_cache_get_response_status("key_1", ECacheGetResponseStatus.MISS),
                        _get_cache_get_response_status("key_128", ECacheGetResponseStatus.HIT),
                    ]
                }
            )

            expected_s3_audio_http_requests = [(i, f"/some_path?param={i}") for i in (1,)]
            expected_tts_backend_requests = []
            response = await stream.read(timeout=1.0)
            _check_response(
                response,
                expected_s3_audio_http_requests,
                expected_tts_backend_requests,
            )
