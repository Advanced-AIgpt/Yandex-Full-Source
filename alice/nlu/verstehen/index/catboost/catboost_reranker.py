import random
import numpy as np

from verstehen.index import Index
from verstehen.index.catboost import CatBoostIndex
from verstehen.index.embedding import DssmKnnIndex, LogregDssmKnnIndex
from verstehen.index.index_registry import registered_index, IndexRegistry


@registered_index
class IndexWithCatboostReranker(Index):
    """
    Index that uses a weak (but presumably fast) index to sample quite small number of hypothesises
    so they can be passed to Catboost index that is considered more powerful yet more time
    consuming.
    """

    DEFAULT_CONFIG = {
        'index_type': 'logreg_with_catboost',

        # name of the index for weak index
        'reuse_weak_index_name': None,
        # or create it from config
        'weak_index_config': LogregDssmKnnIndex.DEFAULT_CONFIG,

        # minimal number of samples to retrieve from small index (if None - maximum number elements are retrieved)
        'min_weak_index_n_samples': 3000,

        # name of the dssm index for catboost features
        'reuse_dssm_index_name': None,
        # or create it from config
        'dssm_index_config': DssmKnnIndex.DEFAULT_CONFIG,

        # catboost parameters (reusage is not viable in this case, as we create new index at each search call)
        'catboost_index_config': CatBoostIndex.DEFAULT_CONFIG
    }

    def __init__(self, weak_index, catboost_config, min_weak_index_n_samples, dssm_index, index_texts):
        self.weak_index = weak_index
        self.catboost_config = catboost_config
        self.min_weak_index_n_samples = min_weak_index_n_samples

        # to pass in Catboost
        self.dssm_index = dssm_index
        self.index_texts = index_texts

    def preprocessing(self, query):
        return query

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        # if one of them None, then retrieve all possible hypothesises
        if self.min_weak_index_n_samples is None or n_samples is None:
            weak_n_samples = None
        else:
            weak_n_samples = max(n_samples, self.min_weak_index_n_samples)

        weak_index_results = self.weak_index.search(preprocessed_query, n_samples=weak_n_samples)
        ids, _ = zip(*weak_index_results)
        catboost_index = self._get_catboost_index(ids)

        preprocessed_query['negative'].extend(self._sample_negatives(len(preprocessed_query['negative'])))

        results = catboost_index.search(
            preprocessed_query, n_samples=n_samples)
        reranking_ids, reranked_scores = zip(*results)
        return list(zip([ids[i] for i in reranking_ids], reranked_scores))

    def estimate_preprocessed(self, preprocessed_query):
        weak_index_results = self.weak_index.search(preprocessed_query)
        ids, _ = zip(*weak_index_results)
        catboost_index = self._get_catboost_index(ids)

        preprocessed_query['negative'].extend(self._sample_negatives(len(preprocessed_query['negative'])))

        results = catboost_index.estimate(preprocessed_query)
        results = sorted(results, key=lambda r: r[0])
        _, scores = zip(*results)
        return scores

    def _get_catboost_index(self, ids):
        ids = np.array(ids, dtype=np.int64)
        sliced_texts = [self.index_texts[idx] for idx in ids]
        return CatBoostIndex(
            sliced_texts,
            self.dssm_index.dssm_applier,
            self.dssm_index._get_index_embeddings()[ids],
            self.catboost_config['min_negative_query_samples'],
            self.catboost_config['cb_depth'],
            self.catboost_config['cb_lr'],
            self.catboost_config['cb_num_trees'],
            self.catboost_config['cb_thread_count'],
            self.catboost_config['cb_val_portion'],
            self.catboost_config['cb_early_stopping_rounds']
        )

    def _sample_negatives(self, cur_negatives_count):
        # we know that Catboost would add some negatives if there is needed to, but we supply a highly
        # positives-concentrated index to it. So, we sample negatives outside the Catboost index from
        # the whole text index that we have.
        negatives_to_sample = self.catboost_config['min_negative_query_samples'] - \
            cur_negatives_count
        sampled = []
        if negatives_to_sample > 0:
            sampled.extend(random.sample(self.index_texts, k=negatives_to_sample))
        return sampled

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        weak_index_name = index_config.get('reuse_weak_index_name', None)
        if weak_index_name is not None:
            weak_index = indexes_map[weak_index_name]
        else:
            weak_index = IndexRegistry.create_index(
                index_config['weak_index_config'], texts, payload=payload, indexes_map=indexes_map
            )

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

        min_weak_index_n_samples = index_config.get(
            'min_weak_index_n_samples', None)

        return IndexWithCatboostReranker(
            weak_index,
            index_config['catboost_index_config'],
            min_weak_index_n_samples,
            dssm_index,
            texts
        )

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        weak_index_name = index_config.get('reuse_weak_index_name', None)
        if weak_index_name is not None:
            weak_index = indexes_map[weak_index_name]
        else:
            weak_index = IndexRegistry.create_index(
                index_config['weak_index_config'],
                texts,
                skill_to_idxs_map,
                payload=payload,
                indexes_map=indexes_map
            )

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

        min_weak_index_n_samples = index_config.get(
            'min_weak_index_n_samples', None)

        skill_to_reranker_map = dict()
        for skill_id, indexes in skill_to_idxs_map.iteritems():
            skill_to_reranker_map[skill_id] = IndexWithCatboostReranker(
                weak_index[skill_id],
                index_config['catboost_index_config'],
                min_weak_index_n_samples,
                dssm_index[skill_id],
                texts[skill_id]
            )
        return skill_to_reranker_map
