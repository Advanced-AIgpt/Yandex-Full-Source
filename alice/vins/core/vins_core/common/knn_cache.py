# coding:utf-8

__knn_cache = None


class DummyKnnCache(object):
    def has(self, key):
        return False

    def put(self, key, value):
        pass

    def get(self, key, default=None):
        return default


def get_knn_cache(default=DummyKnnCache()):
    return __knn_cache or default


def update_knn_cache(cache):
    global __knn_cache
    __knn_cache = cache
