from alice.cachalot.tests.test_cases.util import (
    assert_eq,
)

from alice.cachalot.client import SyncMixin
import alice.cachalot.api.protos.cachalot_pb2 as protos


class CachalotCacheClient(SyncMixin):

    def __init__(self, cachalot):
        self.client = cachalot.get_client(use_grpc=True)

        for method in (
            "cache_get_grpc",
            "cache_set_grpc",
            "cache_del_grpc",
        ):
            setattr(self, method, self._sync_wrapper(getattr(self, '_' + method)))

    async def _cache_get_grpc(self, proto_items, response_types):
        response = await self.client._connection.execute_complex_request("cache_get_grpc", proto_items, response_types)
        self._flattern_oneof(response)

        return response

    async def _cache_set_grpc(self, proto_items, response_types):
        response = await self.client._connection.execute_complex_request("cache_set_grpc", proto_items, response_types)
        self._flattern_oneof(response)

        return response

    async def _cache_del_grpc(self, proto_items, response_types):
        response = await self.client._connection.execute_complex_request("cache_delete_grpc", proto_items, response_types)
        self._flattern_oneof(response)

        return response

    def _flattern_oneof(self, response):
        for key in response:
            response[key] = getattr(response[key], response[key].WhichOneof('Response'))

        return response


def make_get_request(key, storage_tag=None):
    get_request = protos.TGetRequest()
    get_request.Key = key

    if storage_tag is not None:
        get_request.StorageTag = storage_tag

    return get_request


def make_set_request(key, data, storage_tag=None):
    set_request = protos.TSetRequest()
    set_request.Key = key
    set_request.Data = data

    if storage_tag is not None:
        set_request.StorageTag = storage_tag

    return set_request


def make_del_request(key, storage_tag=None):
    del_request = protos.TDeleteRequest()
    del_request.Key = key

    if storage_tag is not None:
        del_request.StorageTag = storage_tag

    return del_request


def assert_set_response(response, key):
    assert_eq(response.Key, key)
    assert not response.Error


def assert_get_response(response, key, data=None, not_found=False):
    assert_eq(response.Key, key)

    if not_found:
        assert not response.Data
    else:
        assert_eq(response.Data, data)


def assert_del_response(response, key):
    assert_eq(response.Key, key)
    assert not response.Error


def test_cache_create_many(cachalot):
    client = CachalotCacheClient(cachalot)

    set_rsp = client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1", b"value_1")),
        ("cache_set_key_2_request", make_set_request("key_2", b"value_2")),
        ("cache_set_key_3_request", make_set_request("key_3", b"value_3")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_set_key_1_request", make_get_request("key_1")),
        ("cache_set_key_2_request", make_get_request("key_2")),
        ("cache_set_key_3_request", make_get_request("key_3")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    assert_set_response(set_rsp["cache_set_key_1_response"], "key_1")
    assert_set_response(set_rsp["cache_set_key_2_response"], "key_2")
    assert_set_response(set_rsp["cache_set_key_3_response"], "key_3")

    assert_get_response(get_rsp["cache_set_key_1_response"], "key_1", b"value_1")
    assert_get_response(get_rsp["cache_set_key_2_response"], "key_2", b"value_2")
    assert_get_response(get_rsp["cache_set_key_3_response"], "key_3", b"value_3")


def test_cache_create_and_update_many(cachalot):
    client = CachalotCacheClient(cachalot)

    client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1", b"value_1_new")),
        ("cache_set_key_2_request", make_set_request("key_2", b"value_2_new")),
        ("cache_set_key_3_request", make_set_request("key_3", b"value_3_new")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    client.cache_set_grpc([
        ("cache_set_key_2_request", make_set_request("key_2", b"value_2_updated")),
        ("cache_set_key_3_request", make_set_request("key_3", b"value_3_updated")),
        ("cache_set_key_4_request", make_set_request("key_4", b"value_4_new")),
    ], {
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
        "cache_set_key_4_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_set_key_1_request", make_get_request("key_1")),
        ("cache_set_key_2_request", make_get_request("key_2")),
        ("cache_set_key_3_request", make_get_request("key_3")),
        ("cache_set_key_4_request", make_get_request("key_4")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
        "cache_set_key_4_response": protos.TResponse,
    })

    assert_get_response(get_rsp["cache_set_key_1_response"], "key_1", b"value_1_new")
    assert_get_response(get_rsp["cache_set_key_2_response"], "key_2", b"value_2_updated")
    assert_get_response(get_rsp["cache_set_key_3_response"], "key_3", b"value_3_updated")
    assert_get_response(get_rsp["cache_set_key_4_response"], "key_4", b"value_4_new")


def test_cache_create_many_and_get_not_existing(cachalot):
    client = CachalotCacheClient(cachalot)

    client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1", b"value_1")),
        ("cache_set_key_2_request", make_set_request("key_2", b"value_2")),
        ("cache_set_key_3_request", make_set_request("key_3", b"value_3")),
        ("cache_set_key_4_request", make_set_request("key_4", b"value_4")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
        "cache_set_key_4_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_set_key_3_request", make_get_request("key_3")),
        ("cache_set_key_4_request", make_get_request("key_4")),
        ("cache_set_key_5_request", make_get_request("key_5")),
        ("cache_set_key_6_request", make_get_request("key_6")),
    ], {
        "cache_set_key_3_response": protos.TResponse,
        "cache_set_key_4_response": protos.TResponse,
        "cache_set_key_5_response": protos.TResponse,
        "cache_set_key_6_response": protos.TResponse,
    })

    assert_get_response(get_rsp["cache_set_key_3_response"], "key_3", b"value_3")
    assert_get_response(get_rsp["cache_set_key_4_response"], "key_4", b"value_4")
    assert_get_response(get_rsp["cache_set_key_5_response"], "key_5", not_found=True)
    assert_get_response(get_rsp["cache_set_key_6_response"], "key_6", not_found=True)


def test_cache_set_get_with_many_option(cachalot):
    client = CachalotCacheClient(cachalot)

    set_rsp = client.cache_set_grpc([
        ("cache_set_request", make_set_request("key_1", b"value_1")),
    ], {
        "cache_set_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_get_request", make_get_request("key_1")),
    ], {
        "cache_get_response": protos.TResponse,
    })

    assert_set_response(set_rsp["cache_set_response"], "key_1")
    assert_get_response(get_rsp["cache_get_response"], "key_1", b"value_1")


def test_cache_create_many_in_different_storages(cachalot):
    client = CachalotCacheClient(cachalot)

    set_rsp = client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1", b"value_Tts",              "Tts")),
        ("cache_set_key_2_request", make_set_request("key_1", b"value_Datasync",         "Datasync")),
        ("cache_set_key_3_request", make_set_request("key_1", b"value_Nonexistent",         "Nonexistent")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_set_key_1_request", make_get_request("key_1", "Tts")),
        ("cache_set_key_2_request", make_get_request("key_1", "Datasync")),
        ("cache_set_key_3_request", make_get_request("key_1", "Nonexistent")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    assert_set_response(set_rsp["cache_set_key_1_response"], "key_1")
    assert_set_response(set_rsp["cache_set_key_2_response"], "key_1")
    assert "cache_set_key_3_response" not in set_rsp

    assert_get_response(get_rsp["cache_set_key_1_response"], "key_1", b"value_Tts")
    assert_get_response(get_rsp["cache_set_key_2_response"], "key_1", b"value_Datasync")
    assert "cache_set_key_3_response" not in get_rsp


def test_cache_create_and_delete_many(cachalot):
    client = CachalotCacheClient(cachalot)

    client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1", b"value_1")),
        ("cache_set_key_2_request", make_set_request("key_2", b"value_2")),
        ("cache_set_key_3_request", make_set_request("key_3", b"value_3")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
    })

    del_rsp = client.cache_del_grpc([
        ("cache_del_key_1_request", make_del_request("key_1")),
        ("cache_del_key_2_request", make_del_request("key_2")),
        ("cache_del_key_3_request", make_del_request("key_3")),
    ], {
        "cache_del_key_1_response": protos.TResponse,
        "cache_del_key_2_response": protos.TResponse,
        "cache_del_key_3_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_get_key_1_request", make_get_request("key_1")),
        ("cache_get_key_2_request", make_get_request("key_2")),
        ("cache_get_key_3_request", make_get_request("key_3")),
    ], {
        "cache_get_key_1_response": protos.TResponse,
        "cache_get_key_2_response": protos.TResponse,
        "cache_get_key_3_response": protos.TResponse,
    })

    assert_del_response(del_rsp["cache_del_key_1_response"], "key_1")
    assert_del_response(del_rsp["cache_del_key_2_response"], "key_2")
    assert_del_response(del_rsp["cache_del_key_3_response"], "key_3")

    assert_get_response(get_rsp["cache_get_key_1_response"], "key_1", not_found=True)
    assert_get_response(get_rsp["cache_get_key_2_response"], "key_2", not_found=True)
    assert_get_response(get_rsp["cache_get_key_3_response"], "key_3", not_found=True)


def test_cache_create_and_delete_some(cachalot):
    client = CachalotCacheClient(cachalot)

    client.cache_set_grpc([
        ("cache_set_key_1_request", make_set_request("key_1_xy", b"value_1")),
        ("cache_set_key_2_request", make_set_request("key_2_xy", b"value_2")),
        ("cache_set_key_3_request", make_set_request("key_3_xy", b"value_3")),
        ("cache_set_key_4_request", make_set_request("key_4_xy", b"value_4")),
    ], {
        "cache_set_key_1_response": protos.TResponse,
        "cache_set_key_2_response": protos.TResponse,
        "cache_set_key_3_response": protos.TResponse,
        "cache_set_key_4_response": protos.TResponse,
    })

    del_rsp = client.cache_del_grpc([
        ("cache_del_key_2_request", make_del_request("key_2_xy")),
        ("cache_del_key_4_request", make_del_request("key_4_xy")),
    ], {
        "cache_del_key_2_response": protos.TResponse,
        "cache_del_key_4_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_get_key_1_request", make_get_request("key_1_xy")),
        ("cache_get_key_2_request", make_get_request("key_2_xy")),
        ("cache_get_key_3_request", make_get_request("key_3_xy")),
        ("cache_get_key_4_request", make_get_request("key_4_xy")),
    ], {
        "cache_get_key_1_response": protos.TResponse,
        "cache_get_key_2_response": protos.TResponse,
        "cache_get_key_3_response": protos.TResponse,
        "cache_get_key_4_response": protos.TResponse,
    })

    assert_del_response(del_rsp["cache_del_key_2_response"], "key_2_xy")
    assert_del_response(del_rsp["cache_del_key_4_response"], "key_4_xy")

    assert_get_response(get_rsp["cache_get_key_1_response"], "key_1_xy", b"value_1")
    assert_get_response(get_rsp["cache_get_key_2_response"], "key_2_xy", not_found=True)
    assert_get_response(get_rsp["cache_get_key_3_response"], "key_3_xy", b"value_3")
    assert_get_response(get_rsp["cache_get_key_4_response"], "key_4_xy", not_found=True)


def test_cache_delete_not_existing(cachalot):
    client = CachalotCacheClient(cachalot)

    del_rsp = client.cache_del_grpc([
        ("cache_del_key_not_exist_request", make_del_request("not_exist")),
    ], {
        "cache_del_key_not_exist_response": protos.TResponse,
    })

    get_rsp = client.cache_get_grpc([
        ("cache_get_key_not_exist_request", make_get_request("not_exist")),
    ], {
        "cache_get_key_not_exist_response": protos.TResponse,
    })

    assert_del_response(del_rsp["cache_del_key_not_exist_response"], "not_exist")
    assert_get_response(get_rsp["cache_get_key_not_exist_response"], "not_exist", not_found=True)
