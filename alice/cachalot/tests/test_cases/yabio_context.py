from alice.cachalot.tests.test_cases import util

import base64


def _grpc_response_getter(rsp):
    return rsp


def _http_response_getter(rsp):
    return rsp["YabioContextResp"]


def _make_response_getter(use_grpc):
    if use_grpc:
        return _grpc_response_getter
    else:
        return _http_response_getter


def test_simple(cachalot, use_grpc):
    client = util.get_client(cachalot, use_grpc=use_grpc)
    getter = _make_response_getter(use_grpc)

    key = ("group-1", "model-1", "yandex")
    data = b"context-1"

    rsp = client.yabio_context_load(*key)
    util.assert_eq(getter(rsp)["Error"]["Status"], "NOT_FOUND")

    rsp = client.yabio_context_save(*key, data)
    util.assert_eq(getter(rsp)["Success"]["Ok"], True)

    rsp = client.yabio_context_load(*key)
    util.assert_eq(getter(rsp)["Success"]["Ok"], True)
    util.assert_eq(base64.b64decode(getter(rsp)["Success"]["Context"]), data)

    rsp = client.yabio_context_delete(*key)
    util.assert_eq(getter(rsp)["Success"]["Ok"], True)

    rsp = client.yabio_context_load(*key)
    util.assert_eq(getter(rsp)["Error"]["Status"], "NOT_FOUND")
