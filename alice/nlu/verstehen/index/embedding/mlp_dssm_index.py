import logging
from time import clock

import numpy as np
from sklearn.neural_network import MLPClassifier

from verstehen.index import Index
from verstehen.index.embedding import DssmKnnIndex
from verstehen.index.index_registry import registered_index
from verstehen.util import top_k_sorted_ids

logger = logging.getLogger(__name__)


@registered_index
class MLPDssmKnnIndex(Index):
    """
    Index that uses underlying DssmKnnIndex which can use query that contains
    positive and negative examples in it by training 2-layers feed-forward net on the
    samples in the query if a specified minimum number of samples in the query
    is surpassed.
    """
    DEFAULT_CONFIG = {
        'index_type': 'mlp_dssm_knn',

        # name of the index for DSSM index
        'reuse_dssm_index_name': None,
        # or create it from config
        'dssm_index_config': DssmKnnIndex.DEFAULT_CONFIG,

        # mlpconfig
        'mlp_min_megative_samples': 50,
        'hidden_size': 50,
        'solver': 'adam',
        'max_iter': 20,
        'batch_size': 1024,
        'test_batch_size': 2048,
        'lr_init': 0.05,
        'early_stopping': False
    }

    def __init__(self, dssm_index, mlp_min_megative_samples=50,
                 hidden_size=50, max_iter=20, solver='adam', batch_size=1024, test_batch_size=2048,
                 lr_init=0.05, early_stopping=False):
        self.dssm_index = dssm_index
        self.mlp_min_megative_samples = mlp_min_megative_samples
        self._classifier = None
        self.hidden_size = hidden_size
        self.max_iter = max_iter
        self.solver = solver
        self.batch_size = batch_size
        self.test_batch_size = test_batch_size
        self.lr_init = lr_init
        self.early_stopping = early_stopping

    def preprocessing(self, query):
        if not isinstance(query, dict):
            if not isinstance(query, (list, tuple, set)):
                query = [query]

            query = {
                'positive': query,
                'negative': []
            }

        return {
            'positive': self.dssm_index.preprocessing(query['positive'])['embeddings'],
            'negative': self.dssm_index.preprocessing(query['negative'])['embeddings']
        }

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        scores = self.estimate_preprocessed(preprocessed_query)
        sorted_ids = top_k_sorted_ids(scores, n_samples)

        return list(zip(sorted_ids, scores[sorted_ids].astype(np.float64)))

    def estimate_preprocessed(self, preprocessed_query):
        start = clock()
        model = self._train_mlp(preprocessed_query)
        end = clock()
        logger.debug('Train: {} s'.format(end - start))
        all_embeddings = self.dssm_index.knn_index.embeddings
        # start = clock()
        n_batches = all_embeddings.shape[0] / self.test_batch_size + 1
        scores = np.zeros((all_embeddings.shape[0],))
        for i in range(n_batches):
            b_start = i * self.test_batch_size
            b_end = min((i + 1) * self.test_batch_size, all_embeddings.shape[0])
            scores[b_start:b_end] = model.predict_proba(all_embeddings[b_start:b_end])[:, 1]
        # end = clock()
        logger.debug('Predict: {} s'.format(end - start))
        return scores

    def _train_mlp(self, preprocessed_query):
        positive, negative = preprocessed_query['positive'], preprocessed_query['negative']

        if len(positive) == 0:
            raise ValueError('Positive query part must not be empty')
        if len(negative) < self.mlp_min_megative_samples:
            to_sample = self.mlp_min_megative_samples - len(negative)
            sampled_negative = self.dssm_index._sample(to_sample)

            if len(negative) > 0:
                negative = np.concatenate(
                    (negative, sampled_negative)
                )
            else:
                negative = sampled_negative

        model = MLPClassifier(
            hidden_layer_sizes=(self.hidden_size,),
            solver=self.solver,
            max_iter=self.max_iter,
            batch_size=self.batch_size,
            learning_rate='adaptive',
            learning_rate_init=self.lr_init,
            early_stopping=self.early_stopping
        )
        # X - data to train on
        X = np.concatenate((positive, negative), axis=0)

        # y - target class prediction. First len(positive) values receive value 1 and for positive samples
        y = np.zeros(shape=(len(positive) + len(negative),))
        y[:len(positive)] = 1

        logger.debug('Training MLPClassifier on {} samples'.format(X.shape[0]))
        model.fit(X, y)
        return model

    @staticmethod
    def get_decision_bound_calculator(intercept):
        def calc_decision_bound(proba):
            return -np.log(1. / proba - 1.) - intercept
        return calc_decision_bound

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        reuse_dssm_index = index_config.get('reuse_dssm_index_name', None)
        if reuse_dssm_index is not None:
            dssm_index = indexes_map[reuse_dssm_index]
        else:
            dssm_index = DssmKnnIndex.from_config(
                index_config['dssm_index_config'],
                texts,
                payload=payload,
                indexes_map=indexes_map
            )

        mlp_min_megative_samples = index_config['mlp_min_megative_samples']
        hidden_size = index_config['hidden_size']
        solver = index_config['solver']
        max_iter = index_config['max_iter']
        batch_size = index_config['batch_size']
        test_batch_size = index_config['test_batch_size']
        lr_init = index_config['lr_init']
        early_stopping = index_config['early_stopping']
        return MLPDssmKnnIndex(
            dssm_index,
            mlp_min_megative_samples=mlp_min_megative_samples,
            hidden_size=hidden_size,
            max_iter=max_iter,
            solver=solver,
            batch_size=batch_size,
            test_batch_size=test_batch_size,
            lr_init=lr_init,
            early_stopping=early_stopping
        )

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        reuse_dssm_index = index_config.get('reuse_dssm_index_name', None)
        if reuse_dssm_index is not None:
            dssm_index = indexes_map[reuse_dssm_index]
        else:
            dssm_index = DssmKnnIndex.from_config(
                index_config['dssm_index_config'],
                texts,
                skill_to_idxs_map,
                payload=payload,
                indexes_map=indexes_map
            )

        mlp_min_megative_samples = index_config['mlp_min_megative_samples']
        hidden_size = index_config['hidden_size']
        solver = index_config['solver']
        max_iter = index_config['max_iter']
        batch_size = index_config['batch_size']
        test_batch_size = index_config['test_batch_size']
        lr_init = index_config['lr_init']
        early_stopping = index_config['early_stopping']
        skill_to_index_map = dict()
        for skill_id, index in dssm_index.items():
            skill_to_index_map[skill_id] = MLPDssmKnnIndex(
                index,
                mlp_min_megative_samples=mlp_min_megative_samples,
                hidden_size=hidden_size,
                max_iter=max_iter,
                solver=solver,
                batch_size=batch_size,
                test_batch_size=test_batch_size,
                lr_init=lr_init,
                early_stopping=early_stopping
            )
        return skill_to_index_map
