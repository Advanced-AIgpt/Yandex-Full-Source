import logging

import numpy as np
from sklearn.linear_model import LogisticRegression

from verstehen.index import Index
from verstehen.index.embedding import DssmKnnIndex
from verstehen.index.index_registry import registered_index
from verstehen.util import nearest_to_const_k_sorted_ids, sample_nearest_to_vector_ids,\
    random_from_batch_ids, top_k_sorted_ids

logger = logging.getLogger(__name__)


@registered_index
class LogregDssmKnnIndex(Index):
    """
    Index that uses underlying DssmKnnIndex which can use query that contains
    positive and negative examples in it by training Logistic Regression on the
    samples in the query if a specified minimum number of samples in the query
    is surpassed. Parameters of the trained Logistic Regression are normalized
    by the L2 norm and then used as an embedding to perform KNN search in the
    underlying DssmKnnIndex.

    The index also supports active_learning regime where the samples returned
    by the index the ones that are the least certain for trained classifier.

    If number of samples in the query does not surpass a specified value then
    no the query is passed directly to DssmKnnIndex without any changes.
    """
    DEFAULT_CONFIG = {
        'index_type': 'logreg_dssm_knn',

        # name of the index for DSSM index
        'reuse_dssm_index_name': None,
        # or create it from config
        'dssm_index_config': DssmKnnIndex.DEFAULT_CONFIG,

        # logistic regression config
        'logreg_min_negative_samples': 50,
        'C': 100.0,
        'active_learning': None
    }

    def __init__(self, dssm_index, logreg_min_negative_samples=50, C=100.0, active_learning=None):
        active_learning_possible_values = [None, 'decision_boundary', 'multiple_boundaries', 'random_upon_bound',
                                           'random_upon_bound_by_probas']
        if active_learning not in active_learning_possible_values:
            raise ValueError(
                'Wrong value for active learning parameter: `{}`. Avalable values are: `{}`'.format(
                    active_learning, active_learning_possible_values
                ))

        self.dssm_index = dssm_index
        self.logreg_min_negative_samples = logreg_min_negative_samples
        self.C = C
        self.active_learning = active_learning
        self._classifier = None

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
        logreg = self._train_logreg(preprocessed_query)

        # Getting parameters of Logistic Regression and normalizing by L2 norm
        emb_query = np.array(logreg.coef_)

        query = {'embeddings': emb_query}
        comparators = []

        if self.active_learning == 'decision_boundary':
            proba = 0.5
            comparators = [LogregDssmKnnIndex._get_near_proba_const_comparator(proba, logreg.intercept_[0])]
        elif self.active_learning == 'multiple_boundaries':
            probas = [1., 0.9, 0.8, 0.7, 0.6, 0.5]  # in probability terms
            comparators = LogregDssmKnnIndex._get_probability_buckets_comparators(probas, logreg.intercept_[0])
        elif self.active_learning == 'random_upon_bound':
            probas = [1., 0.5]
            comparators = LogregDssmKnnIndex._get_probability_buckets_comparators(probas, logreg.intercept_[0])
        elif self.active_learning == 'random_upon_bound_by_probas':
            lower_proba_bound, upper_proba_bound = 0.5, 1.
            comparators = [LogregDssmKnnIndex._get_random_upon_bound_by_probas_comparator(
                lower_proba_bound, upper_proba_bound, logreg.intercept_[0], n_samples
            )]
        else:
            comparators = [top_k_sorted_ids]

        n_comparators = len(comparators)
        if n_samples is not None:
            rough_bucket_size = n_samples // n_comparators
            n_samples_buckets = [
                rough_bucket_size + (1 if i < n_samples % n_comparators else 0) for i in range(n_comparators)
            ]
        else:
            n_samples_buckets = [-1] * n_comparators  # this way we'll never stop reaching n_samples_bucket

        all_ids, all_scores = [], []
        all_ids_set = set()
        for comparator, n_samples_bucket in zip(comparators, n_samples_buckets):
            query['comparator_fn'] = comparator
            search_results = self.dssm_index.search_preprocessed(query, n_samples=n_samples)

            num_added = 0
            for res in search_results:
                idx = res[0]
                score = res[1]
                if idx not in all_ids_set:
                    all_ids_set.add(idx)
                    all_ids.append(idx)
                    all_scores.append(score)
                    num_added += 1

                if num_added == n_samples_bucket:
                    break

        return list(zip(np.array(all_ids), np.array(all_scores)))

    def estimate_preprocessed(self, preprocessed_query):
        logreg = self._train_logreg(preprocessed_query)
        # Getting parameters of Logistic Regression and normalizing by L2 norm
        emb_query = np.array(logreg.coef_)
        query = {'embeddings': emb_query}

        scores = self.dssm_index.estimate_preprocessed(query)
        return scores

    def _train_logreg(self, preprocessed_query):
        positive, negative = preprocessed_query['positive'], preprocessed_query['negative']

        if len(positive) == 0:
            raise ValueError('Positive query part must not be empty')
        if len(negative) < self.logreg_min_negative_samples:
            to_sample = self.logreg_min_negative_samples - len(negative)
            sampled_negative = self.dssm_index._sample(to_sample)

            if len(negative) > 0:
                negative = np.concatenate(
                    (negative, sampled_negative)
                )
            else:
                negative = sampled_negative

        # computing logreg for positive and negative samples
        logreg = LogisticRegression(
            C=self.C
        )
        # X - data to train on
        X = np.concatenate((positive, negative), axis=0)

        # y - target class prediction. First len(positive) values receive value 1 and for positive samples
        y = np.zeros(shape=(len(positive) + len(negative),))
        y[:len(positive)] = 1

        logger.debug('Training logistic regression on {} samples'.format(X.shape[0]))
        logreg.fit(X, y)
        return logreg

    @staticmethod
    def get_decision_bound_calculator(intercept):
        def calc_decision_bound(proba):
            return -np.log(1. / proba - 1.) - intercept
        return calc_decision_bound

    @staticmethod
    def _get_random_upon_bound_by_probas_comparator(lower_proba_bound, upper_proba_bound, intercept, total_n_samples):
        calc_decision_bound_vec = np.vectorize(LogregDssmKnnIndex.get_decision_bound_calculator(intercept))
        probas = calc_decision_bound_vec(np.random.uniform(lower_proba_bound, upper_proba_bound, total_n_samples))
        return lambda cosine_similarity, n_samples: sample_nearest_to_vector_ids(
            cosine_similarity, probas
        )

    @staticmethod
    def _get_near_proba_const_comparator(proba, intercept):
        calc_decision_bound = LogregDssmKnnIndex.get_decision_bound_calculator(intercept)
        return lambda cosine_similarity, n_samples: nearest_to_const_k_sorted_ids(
            cosine_similarity, n_samples, calc_decision_bound(proba)
        )

    @staticmethod
    def _get_probability_buckets_comparators(probas, intercept):
        calc_decision_bound = LogregDssmKnnIndex.get_decision_bound_calculator(intercept)
        comparators = []
        last_proba = probas[0]
        other_proba = probas[1:]
        if not other_proba:
            raise ValueError("you should pass more than one value for probas")

        def _create_random_from_batch_comparator(lower_bound, upper_bound):
            return lambda cosine_similarity, n_samples: random_from_batch_ids(
                cosine_similarity, n_samples, lower_bound, upper_bound
            )

        for proba in other_proba:
            if proba >= last_proba:
                raise ValueError("probas must be sorted in descending order")

            lower_bound = calc_decision_bound(proba)
            upper_bound = calc_decision_bound(last_proba)

            comparators.append(
                _create_random_from_batch_comparator(lower_bound, upper_bound)
            )

            last_proba = proba

        return comparators

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

        logreg_min_negative_samples = index_config['logreg_min_negative_samples']
        C = index_config['C']
        active_learning = index_config['active_learning']
        return LogregDssmKnnIndex(
            dssm_index, logreg_min_negative_samples=logreg_min_negative_samples, C=C, active_learning=active_learning
        )

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        reuse_dssm_index = index_config.get('reuse_dssm_index_name', None)
        if reuse_dssm_index is not None:
            dssm_index = indexes_map[reuse_dssm_index]
        else:
            dssm_index = DssmKnnIndex.from_config_by_skill_id(
                index_config['dssm_index_config'],
                texts,
                skill_to_idxs_map,
                payload=payload,
                indexes_map=indexes_map
            )

        logreg_min_negative_samples = index_config['logreg_min_negative_samples']
        C = index_config['C']
        active_learning = index_config['active_learning']
        skill_to_index_map = dict()
        for skill_id, idx in dssm_index.items():
            skill_to_index_map[skill_id] = LogregDssmKnnIndex(
                idx, logreg_min_negative_samples=logreg_min_negative_samples, C=C, active_learning=active_learning
                )
        return skill_to_index_map
