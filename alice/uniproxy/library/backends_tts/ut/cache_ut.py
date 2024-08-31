# -*- coding: utf-8
from alice.uniproxy.library.backends_tts.cache import (
    cached_tts_lookup_impl, fetch_cached_tts_stream, lookup_result_to_global_counters,
    Cache, CacheManager, CachedTtsStream, LookupResult, UNIPROXY_DISABLE_TTS_PRECACHE_EXP
    )
from alice.uniproxy.library.backends_tts.ttsutils import cache_key

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import UniproxyCounter

from alice.uniproxy.library.testing import run_async
from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.testing.mocks.proto_server import ProtoServer

import voicetech.library.proto_api.ttsbackend_pb2 as protos

from rtlog import null_logger

from tornado import concurrent, gen
from tornado.ioloop import IOLoop


# Helpers ------------------------------------------------------------------------------------------------------------
def create_tts_like_payload(text, voice='shitova', experiments=None):
    d = {
        'format': 'Opus',
        'lang': 'ru-RU',
        'quality': 'UltraHigh',
        'request': {},
        'text': text,
        'voice': voice
    }
    if experiments:
        d['experiments'] = experiments
        d['request']['experiments'] = experiments
    return d


def fake_log_fn(str):
    pass


class FakeCache(Cache):
    def __init__(self, content):
        super(FakeCache, self).__init__("fake")
        self._cache = content


class FakeAsyncCache(object):
    def __init__(self, content):
        self._cache = content
        self.lookup_cnt = 0

    @gen.coroutine
    def lookup(self, key):
        cached = self._cache.get(key, None)
        self.lookup_cnt += 1
        return cached

    @gen.coroutine
    def store(self, key, value):
        self._cache[key] = value


class FakeRTLogger(object):
    def log_child_activation_started(self, child):
        return "fake-token"

    def log_child_activation_finished(self, token, param):
        pass


class FakeTtsProcessor(object):
    def __init__(self):
        self.on_data_calls = 0
        self.from_cache = None
        self.on_error_called = False

    def on_data(self, data, from_cache):
        self.on_data_calls += 1
        self.from_cache = from_cache

    def on_error(self, error):
        self.on_error_called = True


# Tests --------------------------------------------------------------------------------------------------------------
def test_cache_manager_create():
    cache_manager = CacheManager()
    assert(not cache_manager._caches)
    assert(cache_manager._cache_idx == 0)


def test_cache_manager_single_cache():
    cache_manager = CacheManager()
    cache = cache_manager.add_cache()
    assert(cache_manager._caches == {0: cache})
    assert(cache_manager._cache_idx == 1)
    assert(cache.cache_id == 0)
    assert(cache._cache == {})

    assert(cache_manager.get_cache(cache.cache_id) == cache)

    cache_manager.remove_cache(cache)
    assert(cache_manager._caches == {})
    assert(cache_manager._cache_idx == 1)


def test_cache_manager_multiple_caches():
    cache_manager = CacheManager()
    cache_1, cache_2 = cache_manager.add_cache(), cache_manager.add_cache()
    assert(cache_manager._cache_idx == 2)
    assert(cache_1.cache_id == 0)
    assert(cache_1._cache == {})
    assert(cache_2.cache_id == 1)
    assert(cache_2._cache == {})

    assert(cache_manager._caches == {0: cache_1, 1: cache_2})
    assert(cache_manager.get_cache(cache_1.cache_id) == cache_1)
    assert(cache_manager.get_cache(cache_2.cache_id) == cache_2)

    cache_manager.remove_cache(cache_1)
    assert(cache_manager.get_cache(cache_2.cache_id) == cache_2)

    cache_manager.remove_cache(cache_2)
    assert(cache_manager._caches == {})
    assert(cache_manager._cache_idx == 2)


def test_cache_empty():
    cache = Cache(0)
    assert(cache._cache == {})
    assert(cache.lookup(1) is None)


def test_cache_single_item():
    cache = Cache(0)
    cache.store(1, 'value')
    assert(cache.lookup(1) == 'value')


def test_cache_multiple_item():
    cache = Cache(0)
    cache.store(1, 'value_1')
    cache.store(2, 'value_2')
    assert(cache.lookup(1) == 'value_1')
    assert(cache.lookup(2) == 'value_2')


def test_memcache_fail_lookup_no_exp():
    fake_memcache = FakeAsyncCache({})
    cache = FakeCache({})
    rt_log = null_logger()
    request = {'text': 'test'}

    payload = create_tts_like_payload('test')
    key = cache_key(payload)

    cached = IOLoop.instance().run_sync(
        lambda: cached_tts_lookup_impl(fake_memcache, cache, request, payload, rt_log, fake_log_fn))
    assert(cached.result is None)  # Empty cache lookup always fails.
    assert(isinstance(cache.lookup(key), concurrent.Future))

    cached = IOLoop.instance().run_sync(
        lambda: cached_tts_lookup_impl(fake_memcache, cache, request, payload, rt_log, fake_log_fn))
    assert(cached.result is None)
    assert(cached.is_precached is True)
    assert(fake_memcache.lookup_cnt == 1)


def test_memcache_lookup_disable_cache():
    payload = create_tts_like_payload('test')
    payload['disable_cache'] = 1
    key = cache_key(payload)
    fake_memcache = FakeAsyncCache({key: '42'})

    cache = FakeCache({})
    rt_log = null_logger()
    request = {'text': 'test'}

    cached = IOLoop.instance().run_sync(
        lambda: cached_tts_lookup_impl(fake_memcache, cache, request, payload, rt_log, lambda x: print(x)))
    assert(cached.result is None)
    assert(cache.lookup(key) is None)


def test_memcache_succ_lookup():
    payload = create_tts_like_payload('test')
    key = cache_key(payload)
    fake_memcache = FakeAsyncCache({key: '42'})

    cache = FakeCache({})
    rt_log = null_logger()
    request = {'text': 'test'}

    cached = IOLoop.instance().run_sync(
        lambda: cached_tts_lookup_impl(fake_memcache, cache, request, payload, rt_log, lambda x: print(x)))
    assert(cached.result == '42')
    lookup_res = cache.lookup(key)
    assert(isinstance(cache.lookup(key), concurrent.Future))
    assert(lookup_res.result() == '42')


def test_lookup_counters_precache_no_exp():
    UniproxyCounter.init()
    payload = create_tts_like_payload('test', experiments={UNIPROXY_DISABLE_TTS_PRECACHE_EXP: 1})
    lookup_res = LookupResult(payload, None, False)
    lookup_result_to_global_counters(lookup_res)
    assert(GlobalCounter.TTS_PRECACHE_OK_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 0)


def test_lookup_counters_precache_no_exp_2():
    UniproxyCounter.init()
    payload = create_tts_like_payload('test', experiments={UNIPROXY_DISABLE_TTS_PRECACHE_EXP: 1})
    # The situation where the result is precached with a flag is not normal but the behaviour
    # is still as expected.
    lookup_res = LookupResult(payload, '42', True)
    lookup_result_to_global_counters(lookup_res)
    assert(GlobalCounter.TTS_PRECACHE_OK_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 0)


def test_lookup_counters_precache_ok():
    UniproxyCounter.init()
    payload = create_tts_like_payload('test')
    lookup_res = LookupResult(payload, '42', True)
    lookup_result_to_global_counters(lookup_res)
    assert(GlobalCounter.TTS_PRECACHE_OK_SUMM.value() == 1)
    assert(GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 0)


def test_lookup_counters_precache_miss():
    UniproxyCounter.init()
    payload = create_tts_like_payload('test')
    lookup_res = LookupResult(payload, None, False)
    lookup_result_to_global_counters(lookup_res)
    assert(GlobalCounter.TTS_PRECACHE_OK_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MISS_SUMM.value() == 1)


def test_lookup_counters_precache_memcache_miss():
    UniproxyCounter.init()
    payload = create_tts_like_payload('test')
    lookup_res = LookupResult(payload, None, True)
    lookup_result_to_global_counters(lookup_res)
    assert(GlobalCounter.TTS_PRECACHE_OK_SUMM.value() == 0)
    assert(GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.value() == 1)
    assert(GlobalCounter.TTS_PRECACHE_MISS_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 0)
    assert(GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 0)


@run_async()
async def test_failed_tts():
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru.RU",
            "voice": "shitova.gpu",
            "disable_tts_fallback": True,
            "text": "привет человек"
        }
        client = CachedTtsStream("localhost", payload, FakeRTLogger(), fake_log_fn, "fake_message_id")

        # server rejects HTTP-Upgrade
        await srv.pop_proto_stream(accept=False)
        err = await client.get_result(0)
        assert(not isinstance(err, list))


@run_async()
async def test_failed_tts_after_some_audio():
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru",
            "voice": "shitova.gpu",
            "text": "привет человек",
        }

        client = CachedTtsStream("localhost", payload, FakeRTLogger(), fake_log_fn, "fake_message_id")

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/gpu/"
        await srv_stream.read_protobuf(protos.Generate)

        # server sends first audio chunk...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-first-chunk", completed=False
        ))

        # The client receives it...
        res = await client.get_result(0)
        assert(isinstance(res, list))
        assert(len(res) == 1)
        assert(not res[0].completed)

        # ...and then the server closes the stream
        srv_stream.close()

        # client receives the error
        err = await client.get_result(1)
        assert(not isinstance(err, list))


@run_async()
async def test_successfull_tts():
    GlobalCounter.TTS_RU_200_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True
        }

        client = CachedTtsStream("localhost", payload, FakeRTLogger(), fake_log_fn, "fake_message_id")

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        await srv_stream.read_protobuf(protos.Generate)

        # server sends complete audio response...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=True
        ))
        # ...and client receives it
        generate_resp = await client.get_result(0)
        assert isinstance(generate_resp, list)
        assert generate_resp[0].audioData == b"hello-human-complete-audio"
        assert generate_resp[0].completed is True


@run_async()
async def test_successfull_tts_multichunk():
    GlobalCounter.TTS_RU_200_SUMM.set(0)
    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True
        }

        client = CachedTtsStream("localhost", payload, FakeRTLogger(), fake_log_fn, "fake_message_id")

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/"

        # server sends complete audio response...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=False
        ))
        # server sends complete audio response...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=True
        ))
        # ...and client receives it
        await client.get_result(0)
        generate_resp = await client.get_result(1)

        assert isinstance(generate_resp, list)
        assert generate_resp[0].audioData == b"hello-human-complete-audio"
        assert generate_resp[0].completed is True


@run_async()
async def test_fetch_cached_tts_stream_success():
    UniproxyCounter.init()
    cache = FakeCache({})

    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True
        }

        tts = FakeTtsProcessor()
        fut = fetch_cached_tts_stream("localhost", cache, payload, FakeRTLogger(), fake_log_fn, "fake_message_id",
                                      tts.on_data, tts.on_error)

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/"

        # server sends complete audio response...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-complete-audio", completed=True
        ))

        # ...and client receives it
        await fut
        assert(tts.on_data_calls == 1)
        assert(tts.from_cache is False)
        assert(not tts.on_error_called)

    # Re-using the existing cache.
    tts = FakeTtsProcessor()
    await fetch_cached_tts_stream("localhost", cache, payload, FakeRTLogger(), fake_log_fn, "fake_message_id",
                                  tts.on_data, tts.on_error)
    assert(tts.on_data_calls == 1)
    assert(tts.from_cache is True)
    assert(not tts.on_error_called)

    assert GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 1
    assert GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 1


@run_async()
async def test_fetch_cached_tts_stream_error():
    UniproxyCounter.init()
    cache = FakeCache({})

    with ProtoServer() as srv, ConfigPatch({"ttsserver": {"port": srv.port}}):
        payload = {
            "lang": "ru",
            "voice": "shitova",
            "text": "привет человек",
            "disable_tts_fallback": True
        }

        tts = FakeTtsProcessor()
        fut = fetch_cached_tts_stream("localhost", cache, payload, FakeRTLogger(), fake_log_fn, "fake_message_id",
                                      tts.on_data, tts.on_error)

        # server accepts connection and receives "Generate" request
        srv_stream = await srv.pop_proto_stream()
        assert srv_stream.uri == "/ru/"
        generate_req = await srv_stream.read_protobuf(protos.Generate)
        assert generate_req.lang == "ru"
        assert generate_req.text == "привет человек"

        # server sends first audio chunk...
        await srv_stream.send_protobuf(protos.GenerateResponse(
            audioData=b"hello-human-first-chunk", completed=False
        ))

        # ...and then the server closes the stream
        srv_stream.close()

        # client receives the error
        await fut
        assert(tts.on_data_calls == 0)  # The error is given immediately
        assert(tts.from_cache is None)
        assert(tts.on_error_called)

    # Re-using the existing cache.
    tts = FakeTtsProcessor()
    await fetch_cached_tts_stream("localhost", cache, payload, FakeRTLogger(), fake_log_fn, "fake_message_id",
                                  tts.on_data, tts.on_error)
    assert(tts.on_data_calls == 0)
    assert(tts.from_cache is None)
    assert(tts.on_error_called)

    assert GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM.value() == 1
    assert GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM.value() == 1
