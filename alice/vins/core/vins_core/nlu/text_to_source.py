# -*- coding: utf-8 -*-
from __future__ import unicode_literals, absolute_import

import json
import logging

from collections import defaultdict


logger = logging.getLogger(__name__)


def _create_text_to_source(gen, text_to_source_file):
    logger.info('Creating text to source mappings')
    text_to_source = defaultdict(list)
    for nlu_source_item, sample_features in gen:
        text_to_source[sample_features.sample.text].append({
            'source': nlu_source_item.source_path,
            'text': nlu_source_item.text,
            'original_text': nlu_source_item.original_text,
            'trainable_classifiers': nlu_source_item.trainable_classifiers
        })

    logger.info('Dump text to source mappings to %s', text_to_source_file)
    with open(text_to_source_file, mode='w') as fout:
        json.dump(text_to_source, fout)
    return text_to_source


def _stream_samples_from_features_extractor_results(data):
    from vins_core.nlu.base_nlu import FeatureExtractorResult

    for _, results in data.iteritems():
        for result in results:
            if isinstance(result, FeatureExtractorResult):
                yield result.item, result.sample_features


def create_text_to_source_from_features_extractor_results(data, text_to_source_file):
    return _create_text_to_source(_stream_samples_from_features_extractor_results(data), text_to_source_file)


def create_text_to_source_from_feature_cache(feature_cache, text_to_source_file):
    return _create_text_to_source(feature_cache.iterate_all(), text_to_source_file)
