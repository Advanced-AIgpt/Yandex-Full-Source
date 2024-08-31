# coding: utf-8
from __future__ import unicode_literals, absolute_import

import logging
import numpy as np

logger = logging.getLogger(__name__)


class BaseVectorStorage(object):
    """Base class for knn index. Vectors should be normed."""

    _MAX_BATCH_MEMORY = int(1e9)

    def __init__(self, k):
        """
        :param k: number of nearest neighbours
        """

        self._k = k
        self._index_size = None
        self._index = None
        self._step = None

    def _fit_helper(self, vectors):
        raise NotImplementedError

    def _cos_helper(self, vectors):
        raise NotImplementedError

    def fit(self, vectors):
        """Given train vectors fit index. Vectors should be normed."""

        self._index_size = vectors.shape[0]
        self._k = min(self._index_size, self._k)
        self._step = self._MAX_BATCH_MEMORY // self._index_size

        if self._step == 0:
            raise ValueError('Index vectors don\'t fit in memory')

        return self._fit_helper(vectors)

    def predict(self, vectors):
        """Given set of vectors for every vector find its k nearest neighbours
         from index (according to cosine similarity) and calculate mean cosine similarity.
         Vectors should be normed."""

        batch_scores = []

        for i in xrange(0, vectors.shape[0], self._step):
            batch = vectors[i:min(i + self._step, vectors.shape[0])]

            cos = self._cos_helper(batch)
            score = np.mean(cos, axis=1)
            exact_match = np.isclose(np.max(cos, axis=1), 1., atol=1e-7)
            score[exact_match] = 1.

            batch_scores.append(score)

        return np.concatenate(batch_scores)


try:
    from faiss import IndexFlatIP, StandardGpuResources, index_cpu_to_gpu

    class FaissStorage(BaseVectorStorage):
        """Faiss based knn index. Much better than numpy on gpu. Vectors should be normed."""

        def __init__(self, k):
            super(FaissStorage, self).__init__(k)

            self._gpu_resources = StandardGpuResources()

        def _fit_helper(self, vectors):
            self._index = index_cpu_to_gpu(self._gpu_resources, 0, IndexFlatIP(vectors.shape[1]))
            self._index.add(vectors)

            return self

        def _cos_helper(self, vectors):
            D, _ = self._index.search(vectors, self._k)

            return D

    VectorStorage = FaissStorage

    logger.info('Faiss storage loaded')

except ImportError:
    class NumpyStorage(BaseVectorStorage):
        """Numpy based knn index. Vectors should be normed."""

        def _fit_helper(self, vectors):
            self._index = vectors

            return self

        def _cos_helper(self, vectors):
            cos = np.dot(vectors, self._index.T)
            return -np.partition(-cos, self._k - 1, axis=1)[:, :self._k]

    VectorStorage = NumpyStorage

    logger.info('Failed to load Faiss storage (consider running '
                '"pip install -i https://pypi.yandex-team.ru/simple faiss"). Numpy storage loaded')
