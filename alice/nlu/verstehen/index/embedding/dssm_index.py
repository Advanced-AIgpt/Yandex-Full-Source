import logging

import numpy as np
from verstehen.index import Index
from verstehen.index.index_registry import registered_index
from verstehen.preprocess.preprocessing import get_text_preprocessing_fn
from verstehen.util import DssmApplier
from .knn import BruteForceKnnIndex

logger = logging.getLogger(__name__)


@registered_index
class DssmKnnIndex(Index):
    """
    Index that is based on DSSM model which transforms texts to N-dimensional
    embeddings.
    The embeddings are then stored in the inner `self.knn_index` which is used
    to find neighbours in the Euclidean space.

    The queries are also passed as texts to `preprocessing` method where they
    are transformed into a embeddings with the same DSSM model with the same
    dimensionality. The resulting embeddings are used to find the most closest
    ones from the index using previously created `self.knn_index`.
    """

    DEFAULT_CONFIG = {
        'index_type': 'dssm_knn',

        'embedding_dim': None,
        'model_input_name': None,
        'model_output_name': None,

        'embeddings_path': None,
        'model_path': None,
        'text_preprocessing_fn': 'alice_dssm_applier_preprocessing'
    }

    def __init__(self, embeddings, dssm_applier, knn_index_cls, **knn_index_params):
        self.dssm_applier = dssm_applier

        logger.debug('Creating inner KNN index for class {}'.format(
            knn_index_cls.__name__))
        self.knn_index = knn_index_cls(embeddings, **knn_index_params)

    def preprocessing(self, query):
        comparator_fn = None
        if isinstance(query, dict):
            comparator_fn = query.get('comparator_fn', comparator_fn)
            query = query['positive']

        if not isinstance(query, (list, tuple, set)):
            query = [query]

        preprocessed_query = {
            'embeddings': [self.dssm_applier.predict(query_part) for query_part in query]
        }

        if comparator_fn is not None:
            preprocessed_query['comparator_fn'] = comparator_fn

        return preprocessed_query

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        self._get_mean(preprocessed_query)  # should be part of search
        # relying on knn_index API to follow `n_samples` rules, so we don't check sizes of these outputs here
        return self.knn_index.search(preprocessed_query, n_samples=n_samples)

    def estimate_preprocessed(self, preprocessed_query):
        self._get_mean(preprocessed_query)
        return self.knn_index.estimate_preprocessed(preprocessed_query)

    def _get_mean(self, preprocessed_query):
        embeddings = preprocessed_query['embeddings']
        if len(embeddings) == 0:
            raise ValueError('Embeddings must not be empty')
        preprocessed_query['embedding'] = np.mean(embeddings, axis=0)

    def _sample(self, n_sampled):
        return self.knn_index._sample(n_sampled)

    def _get_index_embeddings(self):
        return self.knn_index.embeddings

    @staticmethod
    def from_texts(texts, dssm_applier, knn_index_cls, **knn_index_params):
        logger.debug('Creating {} index on {} texts with KNN index class {}'.format(
            DssmKnnIndex.__name__, len(texts), knn_index_cls.__name__
        ))
        embeddings = [dssm_applier.predict(text) for text in texts]

        return DssmKnnIndex(embeddings, dssm_applier, knn_index_cls, **knn_index_params)

    @staticmethod
    def from_bytes_path(path, emb_dim, dssm_applier, knn_index_cls, **knn_index_params):
        logger.debug('Creating {} index from bytes path {}, with embedding dimensionality {} '
                     'with KNN index class {}'.format(
                         DssmKnnIndex.__name__, path, emb_dim, knn_index_cls.__name__
                     ))
        with open(path, 'rb') as f:
            bytes = f.read()

        n_bytes = len(bytes)
        logger.debug('Read {} bytes from file {}'.format(n_bytes, path))

        # 4 bytes per number in the embedding
        n_embeddings = n_bytes / (emb_dim * 4)
        embeddings = np.frombuffer(
            bytes, dtype=np.float32).reshape(n_embeddings, emb_dim)

        return DssmKnnIndex(embeddings, dssm_applier, knn_index_cls, **knn_index_params)

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        text_preprocessing_fn = get_text_preprocessing_fn(
            index_config['text_preprocessing_fn'])
        dssm_applier = DssmApplier(
            dssm_model_path=index_config['model_path'],
            input_name=index_config['model_input_name'],
            output_name=index_config['model_output_name'],
            text_preprocessing_fn=text_preprocessing_fn
        )
        path = index_config.get('embeddings_path', None)
        if path is not None:
            index = DssmKnnIndex.from_bytes_path(
                path,
                emb_dim=index_config['embedding_dim'],
                dssm_applier=dssm_applier,
                knn_index_cls=BruteForceKnnIndex
            )
        else:
            index = DssmKnnIndex.from_texts(
                texts, dssm_applier=dssm_applier, knn_index_cls=BruteForceKnnIndex
            )
        return index

    @staticmethod
    def from_bytes_path_by_skill_id(path, emb_dim, dssm_applier, knn_index_cls, skill_to_idxs_map, **knn_index_params):
        logger.debug('Creating {} indexes for skills from bytes path {}, with embedding dimensionality {} '
                     'with KNN index class {}'.format(
                         DssmKnnIndex.__name__, path, emb_dim, knn_index_cls.__name__
                     ))
        with open(path, 'rb') as f:
            bytes = f.read()

        n_bytes = len(bytes)
        logger.debug('Read {} bytes from file {}'.format(n_bytes, path))

        # 4 bytes per number in the embedding
        n_embeddings = n_bytes / (emb_dim * 4)
        embeddings = np.frombuffer(
            bytes, dtype=np.float32).reshape(n_embeddings, emb_dim)

        skill_to_index_map = dict()
        for skill_id, indexes in skill_to_idxs_map.items():
            curr_embeddings = [embeddings[i] for i in indexes]
            skill_to_index_map[skill_id] = DssmKnnIndex(curr_embeddings, dssm_applier, knn_index_cls, **knn_index_params)
        return skill_to_index_map

    @staticmethod
    def from_texts_by_skill_id(texts, dssm_applier, knn_index_cls, skill_to_idxs_map, **knn_index_params):
        logger.debug('Creating {} index for skills on {} texts with KNN index class {}'.format(
            DssmKnnIndex.__name__, len(texts), knn_index_cls.__name__
        ))
        skill_to_index_map = dict()
        for skill_id, indexes in skill_to_idxs_map:
            skill_to_index_map[skill_id] = DssmKnnIndex.from_texts(texts[skill_id], dssm_applier, knn_index_cls)

        return skill_to_index_map

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        text_preprocessing_fn = get_text_preprocessing_fn(
            index_config['text_preprocessing_fn'])
        dssm_applier = DssmApplier(
            dssm_model_path=index_config['model_path'],
            input_name=index_config['model_input_name'],
            output_name=index_config['model_output_name'],
            text_preprocessing_fn=text_preprocessing_fn
        )
        path = index_config.get('embeddings_path', None)
        if path is not None:
            skill_to_index_map = DssmKnnIndex.from_bytes_path_by_skill_id(
                path,
                emb_dim=index_config['embedding_dim'],
                dssm_applier=dssm_applier,
                skill_to_idxs_map=skill_to_idxs_map,
                knn_index_cls=BruteForceKnnIndex
            )
        else:
            skill_to_index_map = DssmKnnIndex.from_texts_by_skill_id(
                texts, dssm_applier=dssm_applier, skill_to_idxs_map=skill_to_idxs_map, knn_index_cls=BruteForceKnnIndex
            )
        return skill_to_index_map
