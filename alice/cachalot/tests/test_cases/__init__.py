import time
from functools import partial

from alice.cachalot.tests.test_cases import util


def test_stats_request(cachalot):
    client = util.get_client(cachalot)
    response = client.request_stats()

    assert response is not None
    util.assert_le(0, len(response))


def _test_cache_set_and_get_impl(cachalot, key, value, storage_tag, flags_set, flags_get):
    client = util.get_client(cachalot, use_grpc=True)

    set_rsp = client.cache_set(key, value, 5, storage_tag, **flags_set)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], key)

    get_rsp = client.cache_get(key, storage_tag, **flags_get)
    util.assert_eq(get_rsp["Status"], "OK")
    util.assert_eq(get_rsp["GetResp"]["Key"], key)
    util.assert_eq(get_rsp["GetResp"]["Data"], value)


def _test_cache_set_and_get_same(cachalot, key, value, storage_tag=None, **flags):
    _test_cache_set_and_get_impl(cachalot, key, value, storage_tag, flags_set=flags, flags_get=flags)


def test_cache_set_and_get(cachalot):
    _test_cache_set_and_get_same(cachalot, "rick", b"morty")


def test_cache_set_and_get_utf8(cachalot):
    _test_cache_set_and_get_same(cachalot, "Заяц", "Волк".encode('utf-8'))


def test_cache_set_and_get_imdb_only(cachalot):
    _test_cache_set_and_get_same(cachalot, "air", b"pods", force_imdb=True)


def test_cache_set_and_get_redis_only(cachalot):
    _test_cache_set_and_get_same(cachalot, "beavis", b"butt-head", force_redis=True)


def test_cache_set_and_get_ydb_only(cachalot):
    _test_cache_set_and_get_same(cachalot, "fry", b"bender", force_ydb=True)


def test_cache_set_and_get_tts(cachalot):
    _test_cache_set_and_get_same(cachalot, "Tts_key", b"tts_value", storage_tag="Tts")


def test_cache_set_and_get_datasync(cachalot):
    _test_cache_set_and_get_same(cachalot, "Datasync_key", b"Datasync_value", storage_tag="Datasync")


def test_cache_set_and_get_without_storage_tag(cachalot):
    _test_cache_set_and_get_same(cachalot, "without_storage_tag_key", b"without_storage_tag_value", storage_tag=None)


def test_cache_set_and_get_different_storage_tags(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("Legen", b"Wait for it", 5, "tag_1")
    util.assert_eq(set_rsp["Status"], "CREATED")

    get_rsp = client.cache_get("Dary", "tag_2")
    util.assert_eq(get_rsp["Status"], "NO_CONTENT")


def test_cache_set_all_and_get_ydb(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "paxakor", b"danlark",
        storage_tag=None,
        flags_set=dict(),
        flags_get=dict(force_ydb=True),
    )


def test_cache_set_all_and_get_redis(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "trinity", b"neo",
        storage_tag=None,
        flags_set=dict(),
        flags_get=dict(force_redis=True),
    )


def test_cache_set_ydb_and_get_all(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "morpheus", b"the one",
        storage_tag=None,
        flags_set=dict(force_ydb=True),
        flags_get=dict(),
    )


def test_cache_set_redis_and_get_all(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "Bonnie", b"Clyde",
        storage_tag=None,
        flags_set=dict(force_redis=True),
        flags_get=dict(),
    )


def test_cache_set_all_and_get_imdb(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "cachalot", b"ydb",
        storage_tag=None,
        flags_set=dict(),
        flags_get=dict(force_imdb=True),
    )


def test_cache_set_imdb_and_get_all(cachalot):
    _test_cache_set_and_get_impl(
        cachalot, "Cuttlefish", b"Apphost",
        storage_tag=None,
        flags_set=dict(force_imdb=True),
        flags_get=dict(),
    )


def test_cache_invalid_double_force(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("lisa", b"bart", 5, force_redis=True, force_ydb=True, raise_on_error=False)
    util.assert_eq(set_rsp["Status"], "BAD_REQUEST")

    get_rsp = client.cache_get("lisa", force_redis=True, force_ydb=True, raise_on_error=False)
    util.assert_eq(get_rsp["Status"], "BAD_REQUEST")

    get_rsp = client.cache_get("lisa", force_imdb=True, force_ydb=True, raise_on_error=False)
    util.assert_eq(get_rsp["Status"], "BAD_REQUEST")

    get_rsp = client.cache_get("lisa", force_imdb=True, force_redis=True, force_ydb=True, raise_on_error=False)
    util.assert_eq(get_rsp["Status"], "BAD_REQUEST")


def test_cache_set_redis_and_get_ydb(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("timon", b"pumbaa", 5, force_redis=True)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], "timon")

    get_rsp = client.cache_get("timon", force_ydb=True)
    util.assert_eq(get_rsp["Status"], "NO_CONTENT")


def test_cache_set_ydb_and_get_redis(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("red", b"yellow", 5, force_ydb=True)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], "red")

    get_rsp = client.cache_get("red", force_redis=True)
    util.assert_eq(get_rsp["Status"], "NO_CONTENT")


def test_cache_set_imdb_and_get_ydb(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("Sam Winchester", b"Dean Winchester", 5, force_imdb=True)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], "Sam Winchester")

    get_rsp = client.cache_get("Sam Winchester", force_ydb=True)
    util.assert_eq(get_rsp["Status"], "NO_CONTENT")


def test_cache_set_ydb_and_get_imdb(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("John Winchester", b"Azazel", 5, force_ydb=True)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], "John Winchester")

    get_rsp = client.cache_get("John Winchester", force_imdb=True)
    util.assert_eq(get_rsp["Status"], "NO_CONTENT")


def test_cache_set_and_404(cachalot):
    client = util.get_client(cachalot)

    set_rsp = client.cache_set("bean", b"elfo", 1, force_ydb=True)
    util.assert_eq(set_rsp["Status"], "CREATED")
    util.assert_eq(set_rsp["SetResp"]["Key"], "bean")

    get_rsp = client.cache_get("bean", raise_on_error=True)
    if get_rsp["Status"] != "NO_CONTENT":
        util.assert_eq(get_rsp["Status"], "OK")
        util.assert_eq(get_rsp["Stats"]["BackendStats"][0]["Backend"], "'in-memory lfu storage'")
        util.assert_eq(get_rsp["Stats"]["BackendStats"][0]["Status"], "NO_CONTENT")
        util.assert_eq(get_rsp["Stats"]["BackendStats"][1]["Backend"], "ydb")
        util.assert_eq(get_rsp["Stats"]["BackendStats"][1]["Status"], "OK")


def test_cache_set_too_large_data(cachalot):
    client = util.get_client(cachalot)
    large_data = b"dale" * (1024 * 1024 + 1)
    set_rsp = client.cache_set("chip", large_data, 5, force_ydb=True)
    util.assert_eq(set_rsp["Status"], "BAD_REQUEST", "Wrong status")


def test_cache_charge_redis_from_ydb(cachalot):
    client = util.get_client(cachalot)

    client.cache_set("dead man", b"boots", 5, force_ydb=True)
    client.cache_get("dead man")

    # wait background
    time.sleep(0.5)

    get_rsp = client.cache_get("dead man", force_redis=True)
    util.assert_eq(get_rsp["Status"], "OK")


def test_mm_session(cachalot, use_http):
    client = util.get_client(cachalot, use_grpc=not use_http)

    mm_session_get = partial(client.mm_session_get)
    mm_session_set = partial(client.mm_session_set)

    uuid = f"some_mms_uuid_1_{use_http}"
    dialog_id = f"some_mms_dialog_id_1_{use_http}"
    request_id = f"some_mms_request_id_1_{use_http}"

    rsp = mm_session_get(uuid=uuid, dialog_id=dialog_id, request_id=request_id)
    util.assert_eq(rsp["Status"], "NO_CONTENT")

    rsp = mm_session_set(uuid=uuid, dialog_id=dialog_id, request_id=request_id, data=b"SoMeThInG")
    util.assert_eq(rsp["Status"], "CREATED")

    rsp = mm_session_get(uuid=uuid)
    util.assert_eq(rsp["Status"], "NO_CONTENT")

    rsp = mm_session_get(uuid=uuid, dialog_id=dialog_id, request_id=request_id)
    util.assert_eq(rsp["Status"], "OK")
    util.assert_eq(rsp["MegamindSessionLoadResp"]["Data"], b"SoMeThInG")

    rsp = mm_session_get(uuid=uuid, dialog_id="some_mms_dialog_id_2")
    util.assert_eq(rsp["Status"], "NO_CONTENT")

    rsp = mm_session_get(uuid=uuid, request_id="some_mms_request_id_2")
    util.assert_eq(rsp["Status"], "NO_CONTENT")


def test_mm_session_set_without_request_id(cachalot, use_http):
    client = util.get_client(cachalot, use_grpc=not use_http)

    mm_session_get = partial(client.mm_session_get)
    mm_session_set = partial(client.mm_session_set)

    uuid = "some_mms_uuid_8"
    dialog_id = "some_mms_dialog_id_8"
    request_id = "some_mms_request_id_8"

    rsp = mm_session_get(uuid=uuid, dialog_id=dialog_id, request_id=request_id)
    util.assert_eq(rsp["Status"], "NO_CONTENT")

    rsp = mm_session_set(uuid=uuid, dialog_id=dialog_id, data=b"SoMeThInG")
    util.assert_eq(rsp["Status"], "CREATED")

    rsp = mm_session_get(uuid=uuid)
    util.assert_eq(rsp["Status"], "NO_CONTENT")

    rsp = mm_session_get(uuid=uuid, dialog_id=dialog_id, request_id="")
    util.assert_eq(rsp["Status"], "OK")
    util.assert_eq(rsp["MegamindSessionLoadResp"]["Data"], b"SoMeThInG")


def test_takeout_1(cachalot):
    client = util.get_client(cachalot, use_grpc=False)

    r = client.takeout_set_results([{'job_id': 'job_id1', 'puid': 'puid1', 'texts': ['text1', 'text2']}])
    assert r['Status'] == 'OK'

    r = client.takeout_get_results('job_id1')
    assert r['Status'] == 'OK'
    assert r['TakeoutGetResultsResp']['Success']['Puid'] == 'puid1'
    assert r['TakeoutGetResultsResp']['Success']['Texts'] == ['text1', 'text2']


def test_takeout_2(cachalot):
    client = util.get_client(cachalot, use_grpc=False)

    r = client.takeout_get_results('job_id2')
    assert r['Status'] == 'OK'
    assert r['TakeoutGetResultsResp']['Success']['Puid'] == 'undefined'


def test_takeout_3(cachalot):
    client = util.get_client(cachalot, use_grpc=False)

    r = client.takeout_set_results([{'job_id': 'job_id3', 'puid': 'puid1', 'texts': ['a' * 100] * 100}])
    assert r['Status'] == 'OK'

    r = client.takeout_get_results('job_id3')
    assert r['Status'] == 'OK'
    assert ''.join(r['TakeoutGetResultsResp']['Success']['Texts']) == 'a' * 100 * 100


def _test_cache_delete_after_timeout(cachalot, key1, val1, key2, val2, **flags):
    client = util.get_client(cachalot, use_grpc=True)

    client.cache_set(key1, val1, 1, None, **flags)
    time.sleep(2)
    get_rsp = client.cache_get(key1, None)
    util.assert_eq(get_rsp['Stats']['BackendStats'][0]['Status'], "NO_CONTENT")

    client.cache_set(key2, val2, 2, None, **flags)
    time.sleep(1)
    get_rsp = client.cache_get(key2, None)
    util.assert_eq(get_rsp['Stats']['BackendStats'][0]['Status'], "OK")

    util.assert_eq(get_rsp["GetResp"]["Key"], key2)
    util.assert_eq(get_rsp["GetResp"]["Data"], val2)


def test_cache_delete_after_timeout(cachalot):
    _test_cache_delete_after_timeout(cachalot, "One", b"Two", "Three", b"Four")


def test_cache_delete_after_timeout_imdb_only(cachalot):
    _test_cache_delete_after_timeout(cachalot, "Day", b"Enemy", "Night", b"Friend", force_imdb=True)


# def test_cache_delete_after_timeout_ydb_only(cachalot):
#    _test_cache_delete_after_timeout(cachalot, "Six", b"Seven", "Eight", b"Nine", force_ydb=True)
#
# test does not work because ttl is irrelevant for ydb
