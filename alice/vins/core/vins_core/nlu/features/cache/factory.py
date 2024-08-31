# coding: utf-8

from __future__ import unicode_literals

from vins_core.nlu.features.cache.picklecache import SampleFeaturesCache
from vins_core.nlu.features.cache.protobufcache import YtProtobufFeatureCache


class FeatureCacheFactory(object):
    _caches = {}

    @classmethod
    def create(cls, path):
        if path in cls._caches:
            return cls._caches[path]

        if path is None:
            cache = None
        elif path.startswith('//'):
            cache = YtProtobufFeatureCache(path)
        else:
            cache = SampleFeaturesCache(path)
        cls._caches[path] = cache
        return cache
