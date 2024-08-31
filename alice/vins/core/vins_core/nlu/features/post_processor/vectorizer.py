import logging
import numpy as np
import scipy.sparse

from collections import OrderedDict, defaultdict, Counter
from itertools import izip

from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor
from vins_core.utils.iter import first_of

logger = logging.getLogger(__name__)


class VectorizerFeaturesPostProcessor(BaseFeaturesPostProcessor):

    def __init__(self, sparse=True, sequential=True, return_sample_features=False):
        super(VectorizerFeaturesPostProcessor, self).__init__()
        self.sparse = sparse
        self.sequential = sequential
        self.return_sample_features = return_sample_features

        self._sparse_vectorizer = {}
        self._dense_features = []
        self._dense_seq_features = []

    def _get_feature_name(self, feature, value):
        return '%s=%s' % (feature, value.value)

    def fit(self, batch_features, y=None, **kwargs):
        logger.info('Start fitting vectorizer')
        feature_set = set()
        features_count = defaultdict(list)
        if any(sample_features.sparse_seq for sample_features in batch_features):
            for sample_features in batch_features:
                for feature, feature_values_for_tokens in sample_features.sparse_seq.iteritems():
                    for feature_values in feature_values_for_tokens:
                        for value in feature_values:
                            feature_set.add(self._get_feature_name(feature, value))
                            features_count[feature].append(value.value)
        if any(sample_features.sparse for sample_features in batch_features):
            for sample_features in batch_features:
                for feature, data in sample_features.sparse.iteritems():
                    for value in data:
                        feature_set.add(self._get_feature_name(feature, value))
                        features_count[feature].append(value)
        if feature_set:
            self._sparse_vectorizer = OrderedDict(izip(sorted(feature_set), xrange(len(feature_set))))
            logger.info('%d sparse features are extracted (top10):\n%r', len(feature_set), {
                feature: Counter(values).most_common(10)
                for feature, values in features_count.iteritems()
            })

        sample_features = first_of((sample_features for sample_features in batch_features if sample_features.dense_seq))
        if sample_features:
            self._dense_seq_features.extend(sample_features.dense_seq.keys())
        logger.info('Dense seq features: %r', self._dense_seq_features)

        sample_features = first_of((sample_features for sample_features in batch_features if sample_features.dense))
        if sample_features:
            self._dense_features.extend(sample_features.dense.keys())
        logger.info('Dense features: %r', self._dense_features)
        return self

    def _get_sparse_vectors(self, sample_features):
        # TODO: for models with sequential inputs, it is better to use global features as additional inputs,
        # rather than summing them up by each timestamp
        sparse_idx = []
        for feature, values in sample_features.sparse.iteritems():
            for value in values:
                feature_name = self._get_feature_name(feature, value)
                index = self._sparse_vectorizer.get(feature_name)
                if index is not None:
                    sparse_idx.append(index)

        if self.sequential:
            row, col = [], []
            if sparse_idx:
                for i in sparse_idx:
                    row.extend(range(len(sample_features)))
                    col.extend([i] * len(sample_features))
            for feature, feature_values_for_tokens in sample_features.sparse_seq.iteritems():
                for i, feature_values in enumerate(feature_values_for_tokens):
                    for value in feature_values:
                        feature_name = self._get_feature_name(feature, value)
                        index = self._sparse_vectorizer.get(feature_name)
                        if index is not None:
                            row.append(i)
                            col.append(index)

            output = scipy.sparse.coo_matrix(
                (np.ones(len(row)), (row, col)),
                dtype=np.float32,
                shape=(len(sample_features), len(self._sparse_vectorizer))
            )
        else:
            col = set()
            if sparse_idx:
                col.update(sparse_idx)
            for feature, feature_values_for_tokens in sample_features.sparse_seq.iteritems():
                for _, feature_values in enumerate(feature_values_for_tokens):
                    for value in feature_values:
                        feature_name = self._get_feature_name(feature, value)
                        index = self._sparse_vectorizer.get(feature_name)
                        if index is not None:
                            col.add(index)
            output = scipy.sparse.coo_matrix(
                (np.ones(len(col)), (np.zeros(len(col)), list(col))),
                dtype=np.float32,
                shape=(1, len(self._sparse_vectorizer))
            )
        if self.sparse:
            return output.tocsr()
        else:
            return np.squeeze(output.toarray())

    def _dense_seq_mean(self, dense_seq, sample):
        valid_embeddings = dense_seq[(dense_seq != 0).any(axis=1)]
        if valid_embeddings.size > 0:
            return valid_embeddings.mean(axis=0)
        else:
            logger.debug('Sample "%s" has no valid embeddings. Setting all-zero embedding' % sample.text)
            return np.zeros(valid_embeddings.shape[1])

    def _get_dense_vectors(self, sample_features):

        dense_seq = sample_features.dense_seq_matrix()
        dense = sample_features.dense_matrix()
        if self.sequential and dense.size > 0 and dense_seq.size > 0:
            return np.hstack((dense_seq, np.tile(dense, (len(dense_seq), 1))))
        elif self.sequential and dense.size > 0 and dense_seq.size == 0:
            return np.tile(dense, (len(sample_features), 1))
        elif self.sequential and dense.size == 0 and dense_seq.size > 0:
            return dense_seq
        elif not self.sequential and dense.size > 0 and dense_seq.size > 0:
            return np.hstack((self._dense_seq_mean(dense_seq, sample_features.sample), dense))
        elif not self.sequential and dense.size > 0 and dense_seq.size == 0:
            return dense
        elif not self.sequential and dense.size == 0 and dense_seq.size > 0:
            return self._dense_seq_mean(dense_seq, sample_features.sample)

    def transform(self, batch_features):
        output = []
        logger.debug('Transforming %d sample features into vectors', len(batch_features))
        for sample_features in batch_features:
            vectors = []
            if sample_features.sparse or sample_features.sparse_seq:
                if not self._sparse_vectorizer:
                    raise ValueError('Input features contain one-hots '
                                     'but corresponding vectorizer is not initialized')
                vectors.append(self._get_sparse_vectors(sample_features))
            if sample_features.dense or sample_features.dense_seq:
                vectors.append(self._get_dense_vectors(sample_features))
            if self.sparse:
                if len(vectors) > 1:
                    output.append(scipy.sparse.hstack(vectors).tocsr())
                elif len(vectors) == 1:
                    output.append(scipy.sparse.csr_matrix(vectors[0]))
                else:
                    raise ValueError("Empty features container: %s" % unicode(sample_features))
            else:
                output.append(np.hstack(vectors))

        if not self.sequential:
            output = scipy.sparse.vstack(output) if self.sparse else np.vstack(output)
            logger.debug('Global vectors shape = %r, type = %r', output.shape, type(output))
        else:
            if len(output) > 0:
                logger.debug('Total vectors: %d, vectors dim = %r, type = %r',
                             len(output), output[0].shape[-1], type(output[0]))
        return (output, batch_features) if self.return_sample_features else output

    @property
    def vocabulary(self):
        if self._sparse_vectorizer:
            return self._sparse_vectorizer.keys()
