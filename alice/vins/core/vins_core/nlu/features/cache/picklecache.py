# coding: utf-8

from __future__ import unicode_literals, absolute_import

import attr
import cPickle as pickle
import gc
from itertools import izip
import os
import logging

from vins_core.nlu.features.base import Features, FeatureExtractorResult
from vins_core.nlu.features.cache.base import BaseCache, IterableCache
from vins_core.nlu.features.extractor.custom_serialization import sample_features_to_bytes, sample_features_from_bytes


logger = logging.getLogger(__name__)


def fast_pickle_dump(obj, file_path):
    # Disabling gc is a hack to speed up pickle dumping https://stackoverflow.com/a/9270029
    gc.disable()
    try:
        with open(file_path, mode='wb') as f:
            pickle.dump(obj, f, protocol=pickle.HIGHEST_PROTOCOL)
    finally:
        gc.enable()


class PickleCache(BaseCache):

    def __init__(self, cache_file):
        self._cache = {}
        self._cache_file = cache_file
        self._load()

    def _load(self):
        if self._cache_file:
            if not os.path.exists(self._cache_file):
                logger.warning('Not found cache file %s, it will be created.' % self._cache_file)
            else:
                with open(self._cache_file) as f:
                    self._cache = pickle.load(f)
                logger.info('%d items loaded from cache file %s', len(self._cache), self._cache_file)

    def update(self, inputs, outputs):
        assert len(inputs) == len(outputs)
        num_news = 0
        for input, output in izip(inputs, outputs):
            if input not in self._cache and output is not None:
                self._cache[input] = output
                num_news += 1
        if num_news == 0:
            logger.info('Cache has not changed.')
        else:
            logger.info('Saving %d items in cache: %d newly added', len(self._cache), num_news)
            fast_pickle_dump(self._cache, self._cache_file)
            logger.info('Cache was successfully saved to %s', self._cache_file)
        return outputs

    def __contains__(self, item):
        return item in self._cache

    def _get(self, item):
        return self._cache[item]


@attr.s
class SampleFeaturesCachedResult(object):
    start = attr.ib()
    end = attr.ib()


class SampleFeaturesCache(IterableCache):
    def __init__(self, cache_file):
        self._cache = {}
        self._cache_file = cache_file
        self._data = None
        self._data_file = None

        self._schema_initialized = False
        self._order = {}
        self._info = {}

        self._load()

    @property
    def cache_file(self):
        return self._cache_file

    def _load(self):
        self._data_file = self._cache_file + '.bin'
        if not os.path.exists(self._cache_file):
            logger.warning('Not found cache file %s, it will be created.' % self._cache_file)
        else:
            with open(self._cache_file) as f, open(self._data_file, mode='rb') as fdata:
                self._cache, self._order, self._info = pickle.load(f)
                self._data = fdata.read()

            self._schema_initialized = True

            logger.info('Cache loaded: index from %s, binary data from %s', self._cache_file, self._data_file)

    def _sample_features_to_bytes(self, sample_features):
        if not self._schema_initialized and sample_features:
            raise ValueError('Schema not initialized on writing to cache')

        return sample_features_to_bytes(sample_features, self._order)

    def _bytes_to_sample_features(self, b):
        return sample_features_from_bytes(b, self._order)

    def check_consistency(self, features_extractor, custom_entities=None):
        custom_entities = custom_entities or []

        external_order, external_custom_entities = self._make_metainfo(features_extractor, custom_entities)

        if self._schema_initialized:
            errors = [self._check_features_consistency(external_order),
                      self._check_custom_entities_consistency(external_custom_entities)]
            errors = [e for e in errors if e is not None]

            if len(errors) > 0:
                raise ValueError('Error while checking SampleFeaturesCache consistency:\n{}'
                                 .format('\n\n'.join(errors)))
        else:
            self._order = external_order
            self._info['custom_entities'] = external_custom_entities

            self._schema_initialized = True

            logger.info('Initialize cache {} with:\nFeatures:\n{}\nCustom entities:\n{}'.format(
                self.__class__.__name__,
                '\n'.join('{} order: {}'.format(key.value, value) for key, value in external_order.iteritems()),
                ', '.join(external_custom_entities)))

    def _make_metainfo(self, features_extractor, custom_entities):
        return {key: sorted(value) for key, value in features_extractor.signature.iteritems()}, set(custom_entities)

    def _check_features_consistency(self, external_order):
        not_equal = [
            '{}:\n In cache: {}\n In features extractor: {}'.format(name.value, self._order[name], external_order[name])
            for name in list(Features) if set(external_order[name]) != set(self._order[name])]

        if len(not_equal) > 0:
            return 'Feature types that differ:\n{}'.format('\n'.join(not_equal))
        else:
            return None

    def _check_custom_entities_consistency(self, external_custom_entities):
        internal_custom_entities = self._info['custom_entities']

        if internal_custom_entities ^ external_custom_entities:
            return '(cache custom entities) - (current custom entities) = {}\n' \
                   '(current custom entities) - (cache custom entities) = {}'\
                .format(*[', '.join(entities) for entities in (
                    internal_custom_entities - external_custom_entities,
                    external_custom_entities - internal_custom_entities)])
        else:
            return None

    def _get(self, item):
        result = self._cache[item]
        result = self._get_result(result.start, result.end, item)
        return result

    def _get_result(self, start, end, item):
        chunk = self._data[start:end]
        sample_features = self._bytes_to_sample_features(chunk)
        return FeatureExtractorResult(item=item, sample_features=sample_features)

    def __contains__(self, item):
        return item in self._cache

    def iterate_all(self):
        for item, result in self._cache.iteritems():
            yield item, self._get_result(result.start, result.end, item).sample_features

    def update(self, inputs, outputs):
        logger.info('Updating cache.')

        assert len(inputs) == len(outputs), 'lengths mismatch: len(inputs)=%d, len(outputs)=%d' % (
            len(inputs), len(outputs))

        num_news = 0
        new_data = []
        start = len(self._data) if self._data else 0
        total_items = len(inputs)

        for i in xrange(total_items):
            if not isinstance(outputs[i], (FeatureExtractorResult, SampleFeaturesCachedResult)):
                continue
            if inputs[i] in self._cache:
                if isinstance(outputs[i], SampleFeaturesCachedResult):
                    outputs[i] = self._get_result(outputs[i].start, outputs[i].end, inputs[i])
            else:
                num_news += 1
                b = self._sample_features_to_bytes(outputs[i].sample_features)
                self._cache[inputs[i]] = SampleFeaturesCachedResult(start, start + len(b))
                start += len(b)
                new_data.append(b)

        if num_news == 0:
            logger.info('Cache has not changed.')
        else:
            logger.info('Saving %d items in cache: %d newly added', len(self._cache), num_news)
            fast_pickle_dump((self._cache, self._order, self._info), self._cache_file)

            with open(self._data_file, mode='ab') as fout:
                fout.write(b''.join(new_data))
            logger.info('Cache index saved to %s, data appended to %s', self._cache_file, self._data_file)
        self._load()
        return outputs
