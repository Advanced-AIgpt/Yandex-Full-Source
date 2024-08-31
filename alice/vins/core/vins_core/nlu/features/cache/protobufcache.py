# coding: utf-8

from __future__ import unicode_literals

import itertools
import logging

import yt.wrapper as yt

from vins_core.dm.formats import NluSourceItem
from vins_core.nlu.features.base import SampleFeatures, FeatureExtractorResult
from vins_core.nlu.features.cache.base import IterableCache
from vins_core.utils.config import get_setting


logger = logging.getLogger(__name__)


def _init_yt_client():
    token = get_setting('YT_TOKEN', default=None, prefix='')
    if not token:
        raise EnvironmentError('No YT_TOKEN provided')

    config = {'read_parallel': {
        'enable': True,
        'max_thread_count': get_setting('YT_READ_THREADS', default=20)
    }}
    client = yt.YtClient(
        proxy=get_setting('YT_PROXY', default='hahn', prefix=''),
        token=token,
        config=config,
    )
    return client


class YtProtobufFeatureCache(IterableCache):
    UPDATABLE = False

    def __init__(self, yt_path):
        self.yt_path = yt_path
        self._cache = {}
        if yt_path and yt_path.startswith('//'):
            self._load()

    def _get_key_for_item(self, item):
        assert isinstance(item, NluSourceItem)
        return (item.source_path, item.text)

    def _load(self):
        client = _init_yt_client()
        logger.debug('Start loading yt cache [%s]', self.yt_path)

        for i, row in enumerate(client.read_table(self.yt_path)):
            key = (row['source_path'], row['original_text'].decode('utf-8'))
            self._cache[key] = SampleFeatures.from_bytes(row['sample_features'])
            if i % 1000 == 0:
                logger.debug('Loaded %d cached items', i)
        logger.debug('Cache loading finished sucessfully, %d items loaded', i)

    def __contains__(self, item):
        return self._get_key_for_item(item) in self._cache

    def _get(self, item):
        sf = self._cache[self._get_key_for_item(item)]
        return FeatureExtractorResult(item=item, sample_features=sf)

    def update(self, inputs, outputs):
        for (item, fe_result) in itertools.izip(inputs, outputs):
            self._cache[self._get_key_for_item(item)] = fe_result.sample_features
        return outputs

    def iterate_all(self):
        for (cache_key, sf) in self._cache.iteritems():
            assert isinstance(cache_key, tuple)
            (_, text) = cache_key
            item = NluSourceItem(text=text)
            yield FeatureExtractorResult(item=item, sample_features=sf)
