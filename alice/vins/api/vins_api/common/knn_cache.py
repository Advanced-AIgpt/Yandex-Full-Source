# coding:utf-8


from vins_core.common.knn_cache import DummyKnnCache, update_knn_cache


class RedisKnnCache(DummyKnnCache):
    def __init__(self, redis_connection):
        self._redis_client = redis_connection

    def has(self, key):
        return self._redis_client.exists(key)

    def put(self, key, value):
        self._redis_client.set(key, value)

    def get(self, key, default=None):
        if not self.has(key):
            return default
        return self._redis_client.get(key)


def setup_redis_knn_cache(redis_connection):
    update_knn_cache(RedisKnnCache(redis_connection))
