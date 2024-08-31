import numpy as np

from collections import defaultdict, OrderedDict, Sized
from operator import attrgetter

from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor
from vins_core.nlu.features.post_processor.vectorizer import logger
from vins_core.nlu.features.extractor.base import SparseFeatureValue


class SparseIndexerFeaturesPostProcessor(BaseFeaturesPostProcessor):

    _OOV_SYMBOL = '__OOV__'

    def __init__(self, oov='ignore'):
        """
        Creates unique index for each token presented in sparse / sparse_seq features
        :param oov: takes the following parameters:
                    - "ignore": don't append any index for OOV
                    - "index": process OOV with special token
                    - "error": raise an error when OOV is met
        """

        super(SparseIndexerFeaturesPostProcessor, self).__init__()

        self._set_fields(oov)

    def _set_fields(self, oov, sparse_indexer=None, sparse_seq_indexer=None):
        assert oov in ('ignore', 'index', 'error', None)

        self._sparse_indexer = sparse_indexer or {}
        self._sparse_seq_indexer = sparse_seq_indexer or {}

        self._sparse_seq_features_num = self._get_total_features_num(self._sparse_seq_indexer)
        self._sparse_features_num = self._get_total_features_num(self._sparse_indexer)

        self._oov = oov

    @staticmethod
    def _get_total_features_num(vocab):
        features_num = 0
        for values in vocab.itervalues():
            if isinstance(values, Sized):
                features_num += len(values)
            else:
                features_num += 1
        return features_num

    @property
    def oov(self):
        return self._oov

    @property
    def sparse_seq_indexer(self):
        return self._sparse_seq_indexer

    @property
    def sparse_indexer(self):
        return self._sparse_indexer

    @property
    def sparse_features_num(self):
        return self._sparse_features_num

    @property
    def sparse_seq_features_num(self):
        return self._sparse_seq_features_num

    def _create_tokenizer(self, batch_features, feature_group):
        iter_features = (attrgetter(feature_group)(s) for s in batch_features if hasattr(s, feature_group))
        feature_dict = defaultdict(set)
        logger.info('Collect features...')
        for sample_features in iter_features:
            for feature_name, features in sample_features.iteritems():
                for token in features:
                    if isinstance(token, basestring):
                        feature_dict[feature_name].add(token)
                    elif isinstance(token, (list, tuple)):
                        for feature in token:
                            if isinstance(feature, SparseFeatureValue):
                                feature = feature.value
                            feature_dict[feature_name].add(feature)

        logger.info('Creating vocabulary...')
        tokenizer = OrderedDict()
        features_count = 0
        for feature_name in sorted(feature_dict):
            feature_set = feature_dict[feature_name]
            tokenizer[feature_name] = OrderedDict(zip(
                sorted(feature_set), np.arange(len(feature_set)) + features_count
            ))
            features_count += len(feature_set)

        if self._oov == 'index' and tokenizer:
            logger.info('Appending OOV token...')
            for feature_name in tokenizer:
                tokenizer[feature_name][self._OOV_SYMBOL] = features_count
                features_count += 1

            if features_count:
                # special index for unknown tokens
                tokenizer[self._OOV_SYMBOL] = features_count
                features_count += 1
        return tokenizer, features_count

    def fit(self, batch_features, y=None, **kwargs):
        logger.info('Start fitting sparse tokenizer')
        self._sparse_indexer, self._sparse_features_num = self._create_tokenizer(batch_features, 'sparse')
        logger.info('Start fitting sparse_seq tokenizer')
        self._sparse_seq_indexer, self._sparse_seq_features_num = self._create_tokenizer(batch_features, 'sparse_seq')
        return self

    def _get_value_from_vocab(self, vocab, feature):
        if feature in vocab:
            return vocab[feature]
        if self._oov == 'index':
            return vocab[self._OOV_SYMBOL]
        elif self._oov == 'ignore':
            return None
        elif self._oov == 'error':
            raise KeyError(
                'feature "%s" not found in %s vocabulary.'
                ' Use oov="index" or oov="ignore" to avoid this error' % (feature, self.__class__.__name__)
            )

    def _iter_indices(self, vocab, feature_values):
        for feature_value in feature_values:
            value = self._get_value_from_vocab(vocab, feature_value.value)
            if value is not None:
                yield value

    def _encode_sparse_seq(self, sample_features):
        out = [[] for _ in xrange(len(sample_features))]
        for feature_name, list_of_feature_values in sample_features.sparse_seq.iteritems():
            if feature_name not in self._sparse_seq_indexer:
                oov_value = self._get_value_from_vocab(self._sparse_seq_indexer, feature_name)
                if oov_value is not None:
                    for t in xrange(len(list_of_feature_values)):
                        out[t].append(oov_value)
            else:
                for t, token_feature_values in enumerate(list_of_feature_values):
                    out[t].extend(
                        self._iter_indices(
                            self._sparse_seq_indexer[feature_name], token_feature_values
                        )
                    )
        return out

    def _encode_sparse(self, sample_features):
        out = []
        for feature_name, features in sample_features.sparse.iteritems():
            if feature_name not in self._sparse_indexer:
                oov_value = self._get_value_from_vocab(self._sparse_indexer, feature_name)
                if oov_value is not None:
                    out.append(oov_value)
            else:
                out.extend(self._iter_indices(self._sparse_indexer[feature_name], features))
        return out

    def _encode(self, sample_features, **kwargs):
        out = {}
        if self._sparse_seq_indexer:
            out['sparse_seq'] = self._encode_sparse_seq(sample_features)
        if self._sparse_indexer:
            out['sparse'] = self._encode_sparse(sample_features)
        return out

    def _call(self, batch_features, **kwargs):
        out = []
        for sample_features in batch_features:
            out.append((sample_features, self._encode(sample_features)))
        return out

    def load(self, data):
        self._set_fields(**data)

    def save(self):
        return {
            'oov': self._oov,
            'sparse_indexer': self._sparse_indexer,
            'sparse_seq_indexer': self._sparse_seq_indexer
        }
