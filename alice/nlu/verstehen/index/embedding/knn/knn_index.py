import abc

import numpy as np

from verstehen.index import Index
from verstehen.util import top_k_sorted_ids


class KnnIndex(Index):
    """
    Index for computing the closest samples in the embedding feature space.
    """

    def preprocessing(self, query):
        return query

    @abc.abstractmethod
    def search_preprocessed(self, preprocessed_query, n_samples=None):
        raise NotImplementedError()


class BruteForceKnnIndex(KnnIndex):
    """
    KNN index that uses the dot product for KNN search manually searching
    through all possible candidates.

    NOTE: since the metric for proximity evaluation is dot product, it is the
    developer's responsibility to ensure that the embeddings have norm of 1
    so the dot product is order-equivalent to the inverse Euclidean distance.
    There is no check for the norm size of the embeddings.
    """

    def __init__(self, embeddings, float_type=np.float32):
        """
        Arguments:
            embeddings: numpy array of embeddings or list of embeddings of a fixed dimensionality
            float_type: numpy floating point type used to operate embeddings with
        """

        self.float_type = float_type
        self.index_size = len(embeddings)
        self.index_ids = np.arange(self.index_size, dtype=np.uint32)

        self.embeddings = embeddings

        if not isinstance(self.embeddings, np.ndarray):
            self.embeddings = np.array(self.embeddings, dtype=float_type)

        if not self.embeddings.dtype == float_type:
            self.embeddings = self.embeddings.astype(float_type)

    def search_preprocessed(self, query, n_samples=None):
        comparator = top_k_sorted_ids

        if isinstance(query, dict):
            comparator = query.get('comparator_fn', comparator)

        if n_samples is None or n_samples > self.index_size:
            n_samples = self.index_size

        cosine_similarity = self.estimate_preprocessed(query)
        # getting top n_samples results since more samples would not be needed
        sorted_idx = comparator(cosine_similarity, n_samples)

        return list(zip(sorted_idx, cosine_similarity[sorted_idx].astype(np.float64)))

    def estimate_preprocessed(self, query):
        if isinstance(query, dict):
            preprocessed_query = query.get('embedding')
        else:
            preprocessed_query = query

        embedding = np.array(preprocessed_query, dtype=self.float_type)

        if len(embedding.shape) == 1:
            embedding = np.expand_dims(embedding, axis=0)

        cosine_similarity = np.matmul(self.embeddings, embedding.T).squeeze(axis=1)  # calculating proximity
        return cosine_similarity.astype(np.float64)

    def _sample(self, n_sampled):
        return self.embeddings[np.random.randint(self.embeddings.shape[0], size=n_sampled), :]
