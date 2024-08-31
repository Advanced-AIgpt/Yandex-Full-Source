import os

import numpy as np
import tempfile

from hnsw import Pool, Hnsw, EVectorComponentType, EDistance

from .knn_index import KnnIndex


class HnswKnnIndex(KnnIndex):
    """
    KNN index that utilities the HNSW library for fast approximate KNN search
    using dot product.

    NOTE: since the metric for proximity evaluation is dot product, it is the
    developer's responsibility to ensure that the embeddings have norm of 1
    so the dot product is order-equivalent to the inverse Euclidean distance.
    There is no check for the norm size of the embeddings.

    NOTE: Still under development.
    """

    def __init__(self, embeddings, max_neighbours, search_neighborhood_size, batch_size, num_exact_candidates,
                 report_progress=False):
        self.num_exact_candidates = num_exact_candidates
        self.batch_size = batch_size
        self.search_neighborhood_size = search_neighborhood_size
        self.max_neighbours = max_neighbours
        self.report_progress = report_progress

        embeddings = np.array(embeddings, dtype=np.float32)
        if not len(embeddings.shape) == 2:
            raise ValueError(
                'Embeddings should be a list of equally sized 1-dimensional arrays')

        embeddings_bytes = embeddings.tobytes()
        tmpfile = tempfile.mktemp()
        try:
            with open(tmpfile, 'w') as f:
                f.write(embeddings_bytes)

            self.length = embeddings.shape[0]
            self.dimension = embeddings.shape[1]
            pool = Pool(vectors_path=tmpfile,
                        dtype=EVectorComponentType.Float, dimension=self.dimension)
            self.hnsw = Hnsw()
            self.hnsw.build(pool, EDistance.DotProduct, max_neighbors=self.max_neighbours,
                            search_neighborhood_size=self.search_neighborhood_size, batch_size=self.batch_size,
                            num_exact_candidates=self.num_exact_candidates, report_progress=self.report_progress)
        finally:
            os.remove(tmpfile)

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        # conversion is needed for the HNSW library
        embedding = np.array(preprocessed_query, dtype=np.float32)
        if n_samples is None or n_samples > self.length:
            n_samples = self.length

        neighbors = self.hnsw.get_nearest(embedding, top_size=n_samples,
                                          search_neighborhood_size=self.search_neighborhood_size)
        ids, scores = zip(*neighbors)
        return list(zip(np.array(ids, dtype=np.int), np.array(scores)))

    def estimate_preprocessed(self, preprocessed_query):
        # conversion is needed for the HNSW library
        embedding = np.array(preprocessed_query, dtype=np.float32)

        neighbors = self.hnsw.get_nearest(embedding, top_size=self.length,
                                          search_neighborhood_size=self.search_neighborhood_size)
        neighbors = sorted(neighbors, key=lambda n: n[0])
        ids, scores = zip(*neighbors)
        return scores
