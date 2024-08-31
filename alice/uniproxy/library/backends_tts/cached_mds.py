from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import GlobalCounter
from tornado.ioloop import IOLoop
from tornado.httpclient import AsyncHTTPClient, HTTPRequest
from time import monotonic
from math import inf
from cachetools import LRUCache


class AsyncCacheWithTtl:
    """ LRU cache with lazy updates on TTL via async function
    Length of data is used to measure cache's size
    """

    @staticmethod
    def getsizeof(item):
        return len(item[1])

    def __init__(self, func, maxsize, ttl=60):
        self._cache = LRUCache(
            maxsize=maxsize,
            getsizeof=self.getsizeof
        )
        self._ttl = ttl
        self._func = func

    async def _update(self, key):
        try:
            val = await self._func(key)
            prev_size = self._cache.currsize
            self._cache[key] = [monotonic() + self._ttl, val]  # must be mutable

            # NOTE: since the cache isn't shared among processes but YASM-signal is, we can't
            # simply set new size - all changes must be made via deltas
            GlobalCounter.S3_AUDIO_CACHE_SIZE_MAX.increment(self._cache.currsize - prev_size)

            return val
        except:
            item = self._cache.get(key)
            if item is not None:
                item[0] = 0  # mark item expired
            raise

    async def get(self, key):
        item = self._cache.get(key)
        if item is None:
            return await self._update(key)

        if item[0] < monotonic():  # item is expired
            item[0] = inf  # to prevent additional updates
            IOLoop.current().spawn_callback(self._update, key)

        return item[1]


class CachedMds:
    def __init__(self):
        url_template = config["tts"]["s3_url"]
        request_timeout = config["tts"]["s3_timeout"]
        logger = Logger.get(".tts.cached-mds")

        cache_maxsize = config["tts"].get("s3_cache_maxsize", 1024*1024*32)  # 32Mb
        cache_ttl = config["tts"].get("s3_cache_ttl", 60)  # 1 minute

        async def get_content(key):
            url = url_template % key
            try:
                response = await AsyncHTTPClient().fetch(
                    request=HTTPRequest(url=url, request_timeout=request_timeout)
                )
                logger.debug(f"Obtained content of '{key}', url='{url}'")
                return response.body
            except:
                logger.exception(f"Failed to obtain content of '{key}', url='{url}'")
                raise

        self._cache = AsyncCacheWithTtl(
            func=get_content,
            maxsize=cache_maxsize,
            ttl=cache_ttl
        )

    async def get(self, key):
        return await self._cache.get(key)


_global_instance = None


async def get_from_cached_mds(key, nothrow=False):
    global _global_instance

    if _global_instance is None:
        _global_instance = CachedMds()

    try:
        return await _global_instance.get(key)
    except:
        if nothrow:
            return None
        raise


# for tests
def reset_cached_mds():
    global _global_instance
    _global_instance = None
