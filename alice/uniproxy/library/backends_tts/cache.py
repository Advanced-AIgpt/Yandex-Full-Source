import time

from alice.cachalot.client import CachalotClient

from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.settings import config

from alice.uniproxy.library.backends_tts.ttsutils import cache_key, format2mime, get_language, mime2format
from alice.uniproxy.library.backends_tts.ttsutils import split_text_by_speaker_tags
from alice.uniproxy.library.backends_tts.ttsutils import override_voice_if_needed

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import conducting_experiment, rtlog_child_activation

from collections import namedtuple
from alice.uniproxy.library.utils.deepcopyx import deepcopy

from tornado import gen

from .ttsstream import TtsStream


UNIPROXY_CACHE_TIMEOUT = config['tts'].get('cache', {}).get('timeout', 0.1)
UNIPROXY_CACHE_MAX_TEXT = config["tts"].get("cache", {}).get("maxchars", 30)
UNIPROXY_DISABLE_TTS_PRECACHE_EXP = 'uniproxy_disable_tts_precache'
UNIPROXY_FULL_TTS_PRECACHE_EXP = 'uniproxy_full_tts_precache'


LookupResult = namedtuple('LookupResult', ['payload', 'result', 'is_precached'])


def lookup_result_to_global_counters(cache_lookup_res: LookupResult):
    if conducting_experiment(UNIPROXY_DISABLE_TTS_PRECACHE_EXP, cache_lookup_res.payload):
        return

    if cache_lookup_res.is_precached:
        if cache_lookup_res.result is not None:
            GlobalCounter.TTS_PRECACHE_OK_SUMM.increment()
        else:
            GlobalCounter.TTS_PRECACHE_MEMCACHE_MISS_SUMM.increment()
    else:
        GlobalCounter.TTS_PRECACHE_MISS_SUMM.increment()


class Cache(object):
    def __init__(self, cache_id):
        self._cache = {}
        self.cache_id = cache_id

    def lookup(self, key):
        cached = self._cache.get(key, None)
        return cached

    def get_or_create(self, key, constructor):
        cached = self._cache.get(key, None)
        is_new = cached is None
        if is_new:
            cached = self._cache[key] = constructor(key)
        return (is_new, cached)

    def store(self, key, value):
        self._cache[key] = value


# --------------------------------------------------------------------------------------------------------------------
class CacheManager(object):
    def __init__(self):
        self._caches = {}
        self._cache_idx = 0

    def add_cache(self):
        id = self._cache_idx
        res = Cache(id)
        self._caches[id] = res
        self._cache_idx += 1
        return res

    def remove_cache(self, cache):
        del self._caches[cache.cache_id]

    def get_cache(self, cache_id):
        return self._caches.get(cache_id, None)


class CacheStorageClient:
    def __init__(self):
        self.cachalot_client = make_cachalot_client()

    async def lookup(self, key):
        with UnistatTiming('tts_cache_post_time'):
            response = await self.cachalot_client.cache_get(key, raise_on_error=False)

        if response is not None and response["Status"] == "OK":
            GlobalCounter.TTS_CACHE_GET_OK_SUMM.increment()
            return response["GetResp"]["Data"]
        else:
            GlobalCounter.TTS_CACHE_GET_MISS_SUMM.increment()
            return None

    async def store(self, key, value, ttl=None):
        # Use ttl only in special cases!
        with UnistatTiming('tts_cache_get_time'):
            await self.cachalot_client.cache_set(key, value, ttl=ttl, raise_on_error=False)
        GlobalCounter.TTS_CACHE_POST_SUMM.increment()


def make_cachalot_client():
    cachlot_config = config['cachalot']
    return CachalotClient(cachlot_config['host'], cachlot_config['http_port'])

# --------------------------------------------------------------------------------------------------------------------


@gen.coroutine
def timed_lookup(cache, key, metrics, lookup_name):
    start_time = time.monotonic()
    res = yield cache.lookup(key)
    finish_time = time.monotonic()
    if metrics:
        if metrics[lookup_name] is None:
            metrics[lookup_name] = 0
        metrics[lookup_name] += finish_time - start_time
    return res


# --------------------------------------------------------------------------------------------------------------------
@gen.coroutine
def timed_memcache_lookup(memcache, key, key_text, metrics, rt_log):
    with rtlog_child_activation(rt_log, 'tts.query_cache'):
        res = yield timed_lookup(memcache, key, metrics, 'memcache_time')
    rt_log('tts.query_cache_finished', size=len(res) if res is not None else None)
    return res


class MemCacheCtor(object):
    def __init__(self, memcache, key_text, metrics, rt_log, log_fn):
        self._memcache = memcache
        self._key_text = key_text
        self._metrics = metrics
        self._rt_log = rt_log
        self._log_fn = log_fn

    def __call__(self, key):
        self._log_fn("Starting memcache lookup for: {}".format(self._key_text))
        return timed_memcache_lookup(self._memcache, key, self._key_text, self._metrics, self._rt_log)


# --------------------------------------------------------------------------------------------------------------------
def is_cache_disabled(tts_like_payload):
    return tts_like_payload.get('disable_cache', False)


# --------------------------------------------------------------------------------------------------------------------
@gen.coroutine
def cached_tts_lookup_impl(memcache, cache, request, tts_like_payload, rt_log, log_fn,
                           metrics=None) -> LookupResult:
    key_text = request['text']

    payload = tts_like_payload.copy()
    payload.update(request)
    override_voice_if_needed(payload)

    key = cache_key(payload)

    if not is_cachable(key_text):
        log_fn("Text is not cachable: {}".format(key_text))
        return LookupResult(payload, result=None, is_precached=False)

    if is_cache_disabled(payload):
        log_fn("Memcache lookup is disabled for this request")
        return LookupResult(payload, result=None, is_precached=False)

    memcache_ctor = MemCacheCtor(memcache, key_text, metrics, rt_log, log_fn)
    is_new, future = cache.get_or_create(key, memcache_ctor)
    res = None
    if future is not None:
        res = yield future

    # If there is an entry of any kind (present or not), return it.
    log_fn("Precached text: {}, key: {}, len: {}".format(key_text, key, len(res) if res is not None else None))
    return LookupResult(payload, result=res, is_precached=not is_new)


# --------------------------------------------------------------------------------------------------------------------
@gen.coroutine
def memcache_tts_lookup(cache, request, tts_like_payload, rt_log, log_fn, metrics=None):
    memcache = CacheStorageClient()
    res = yield cached_tts_lookup_impl(memcache, cache, request, tts_like_payload, rt_log, log_fn,
                                       metrics=metrics)
    return res


# --------------------------------------------------------------------------------------------------------------------
class CachedTtsStream(object):
    def __init__(self, host, stream_payload, rt_log, log_fn, message_id):
        self._partial_results = []
        self._futures = []
        self._error = None
        self._stream_payload = stream_payload
        self._rt_log = rt_log
        self._log_fn = log_fn
        self._message_id = message_id
        self._is_completed = False

        self._tts_stream = TtsStream(
            self._on_data,
            self._on_tts_error,
            self._stream_payload,
            host=host,
            rt_log=self._rt_log,
            message_id=self._message_id,
        )

    def _wake_all(self):
        for future in self._futures:
            future.set_result(1)

    def _on_data(self, res, *args, **kwargs):
        self._log_fn("Caching TTS chunk #{}".format(len(self._partial_results)))
        self._partial_results.append(res)
        self._wake_all()

        if res.completed:
            self._is_completed = True

    def _on_tts_error(self, error):
        self._log_fn("TTS error: {}".format(error))
        self._error = error
        self._wake_all()

    @gen.coroutine
    def get_result(self, start_idx):
        if self._error:
            return self._error

        if start_idx < len(self._partial_results):
            res = self._partial_results[start_idx:]
            self._log_fn("Getting %d existing elements from %d" % (len(res), start_idx))
            return res

        assert(start_idx == len(self._partial_results))  # Streaming elements "from the future" is forbidden.
        assert(not self._is_completed)  # If the last chunk is read and is completed, there should be nothing to get.

        self._log_fn("No elements available, waiting %d" % start_idx)
        future = gen.Future()
        future.add_done_callback(lambda x: self._futures.remove(future))
        self._futures.append(future)
        yield future
        if self._error:
            return self._error
        return self._partial_results[start_idx:]


class TtsStreamCtor(object):
    def __init__(self, host, stream_payload, rt_log, log_fn, message_id):
        self._tts_stream_rtlog_token = None
        self._stream_payload = stream_payload
        self._host = host
        self._rt_log = rt_log
        self._log_fn = log_fn
        self._message_id = message_id

    def __call__(self, key):
        self._log_fn("Creating a new TTS stream for %s" % key)
        return CachedTtsStream(self._host, self._stream_payload, self._rt_log, self._log_fn, self._message_id)


# --------------------------------------------------------------------------------------------------------------------
@gen.coroutine
def fetch_cached_tts_stream(host, tts_cache, stream_payload, rt_log, log_fn, message_id, on_data, on_error):
    key = cache_key(stream_payload)
    tts_ctor = TtsStreamCtor(host, stream_payload, rt_log, log_fn, message_id)
    is_new, tts_cache_entry = tts_cache.get_or_create(key, tts_ctor)
    counter = GlobalCounter.TTS_FULL_PRECACHE_FAIL_SUMM if is_new else GlobalCounter.TTS_FULL_PRECACHE_OK_SUMM
    counter.increment()

    start_idx = 0
    while True:
        fetched = yield tts_cache_entry.get_result(start_idx)
        if isinstance(fetched, list):
            nelem = len(fetched)
            log_fn("Got tts results with %d elements" % nelem)
            start_idx += nelem
            for item in fetched:
                on_data(item, from_cache=not is_new)
                if item.completed:
                    return
        else:  # error
            log_fn("Got tts error: %s" % fetched)
            on_error(fetched)
            return


# --------------------------------------------------------------------------------------------------------------------
class TTSPreloader(object):
    def __init__(self, cache, tts_cache, system, rt_log, message_id, on_close=None):
        self._cache = cache
        self._tts_cache = tts_cache
        self._on_close = on_close
        self._closed = False
        self._rt_log = rt_log
        self._system = system
        self._log = Logger.get('.tts.preload')
        self._message_id = message_id
        self._initial_stream_payload = None
        self._stream_payload = None
        self._prefetch_queue = None

    def close(self):
        if not self._closed:
            self._closed = True
            if self._on_close:
                self._on_close()

    def log(self, msg):
        self._log.debug(msg, rt_log=self._rt_log)

    def start(self, tts_like_payload):
        payload_with_session_data = deepcopy(tts_like_payload)

        d = mime2format(payload_with_session_data)
        text = d.pop("text", None)

        self._initial_stream_payload = d.copy()
        self._initial_stream_payload["lang"] = get_language(d.get("lang", "ru"))
        self._initial_stream_payload["mime"] = format2mime(d)

        self._stream_payload = deepcopy(self._initial_stream_payload)

        if text is not None:
            self._prefetch_queue = split_text_by_speaker_tags(text, payload_with_session_data.get('settings_from_manager', dict()))
            self.cache()
        self.close()

    @gen.coroutine
    def cache(self):
        if is_cache_disabled(self._initial_stream_payload):
            return

        self.log("Starting precache for: %s" % self._prefetch_queue)

        for request in self._prefetch_queue:
            if self._closed:
                return

            self.log("Starting prefetch for: %s" % request)
            precached = yield memcache_tts_lookup(self._cache, request, self._stream_payload,
                                                  self._rt_log, self.log)

            # https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/processors/tts.py?rev=r8177632#L560-563
            self._stream_payload = precached.payload

            if not precached.result and not self._closed and \
                    conducting_experiment(UNIPROXY_FULL_TTS_PRECACHE_EXP, precached.payload):
                self.log("Precache via memcache failed, trying to precache real TTS response for %s" % request)
                key = cache_key(precached.payload)
                tts_action = TtsStreamCtor(self._system.srcrwr['TTS'], precached.payload, self._rt_log, self.log,
                                           self._message_id)
                self._tts_cache.get_or_create(key, tts_action)


# --------------------------------------------------------------------------------------------------------------------
def is_cachable(text):
    return len(text) <= UNIPROXY_CACHE_MAX_TEXT
