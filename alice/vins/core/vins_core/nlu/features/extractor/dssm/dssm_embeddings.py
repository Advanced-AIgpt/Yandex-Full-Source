# coding: utf-8
from __future__ import unicode_literals

import os
import logging
import cPickle as pickle
import numpy as np

from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, DenseFeatures
from vins_core.nlu.features.extractor.dssm.utils import get_apply_feed
from vins_core.nlu.features.extractor.dssm.config import Context1RNNVinsdict
from vins_core.utils.data import get_resource_full_path

from vins_models_tf import TfEncoder

logger = logging.getLogger(__name__)


class DSSMApplier(object):
    def __init__(self, config, resource):
        dir_path = self._download_model(config, resource)
        self._preprocessor = config.preprocessor(config=config, dicts_path=dir_path)
        self._model = TfEncoder(dir_path)
        with open(os.path.join(dir_path, 'encoders_list.pkl'), 'rb') as f:
            self._encoders = pickle.load(f)

    def encode(self, batch):
        preprocessed_batch = self._preprocess(batch)
        feed = self._get_feed(preprocessed_batch)
        return self._model.encode(feed)

    def _preprocess(self, batch):
        preprocessed_batch = [self._preprocessor.parse_context(value) for value in batch]
        keys = preprocessed_batch[0].keys()
        return {
            key: np.asarray([
                batch[key] for batch in preprocessed_batch
            ], dtype=list) for key in keys
        }

    def _get_feed(self, preprocessed_batch):
        feed = {}
        for name, encoder in self._encoders:
            feed.update(get_apply_feed(encoder, preprocessed_batch[name]))
        return feed

    def _download_model(self, config, resource):
        dir_name = None
        for file_name in config.MODEL_FILES:
            download_path = get_resource_full_path(resource + '/' + file_name)
            if not dir_name:
                dir_name = os.path.dirname(download_path)
            else:
                assert dir_name == os.path.dirname(download_path)
        return dir_name


class DssmEmbeddingsFeaturesExtractor(BaseFeatureExtractor):

    def __init__(self, resource, num_words=None, num_trigrams=None, words_index=None, **kwargs):
        super(DssmEmbeddingsFeaturesExtractor, self).__init__()

        self._config = Context1RNNVinsdict(
            word_dct_size=num_words,
            trigram_dct_size=num_trigrams,
            keep_word_indices=words_index
        )
        logger.info('Config created...')

        self._model = DSSMApplier(self._config, resource)

    def _get_name(self):
        return 'dssm_embeddings_%s' % self._config

    def _call(self, sample, **kwargs):
        result = self._model.encode([[sample.text]])[0]
        return [DenseFeatures(result)]

    @property
    def _features_cls(self):
        return DenseFeatures
