from verstehen.index import Index
from verstehen.index.index_registry import registered_index


@registered_index
class CompositeIndex(Index):
    """
    Index that is able to compose many existing indexes into one working index.
    The index passes the calls to underlying indexes and then unites the results.

    The results are then united in the following manner: the underlying indexes gives a
    list of results that are sorted by the score in the descending order. The merged
    result is a list where the first N items are top-1 prediction from N underlying
    indexes in the composite index, next N items are top-2 prediction from N underlying
    indexes and so on. After merging, the results are filtered for the duplicates of the
    same ids found by different underlying indexes (which is very common to happen). The
    duplicates are removed so only the first its occurrence remains.

    The work of the index can be described in the following way:

        Underlying indexes returned the following results:
        [index1_top1, index1_top2, ..., index1_topM], ... , [indexN_top1, ..., indexN_topM]

        Merged results:
        [index1_top1, index2_top1, index3_top1, ..., indexN-1_topM, indexN_topM]

        Filtered merged resutts (duplicates removal):
        [index1_top1, index2_top1, index3_top1, index1_top2, index1_top3, index2_top3, ...]


    NOTE: since the scores from indexes might have different nature, the resulting response
    by the search call might (and probably will) not return scores in the sorted way.
    """

    DEFAULT_CONFIG = {
        'index_type': 'composite',

        # names of indexes to use in composite index
        'reuse_indexes': []
    }

    def __init__(self, indexes):
        # copying the indexes into list to have unchanging and reliable order-wise iteration
        self.indexes = list(indexes)

    def preprocessing(self, query):
        preprocessed = []
        for index in self.indexes:
            preprocessed.append(index.preprocessing(query))
        return preprocessed

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        ids, scores = [], []
        for index, prep_query in zip(self.indexes, preprocessed_query):
            index_results = index.search_preprocessed(prep_query, n_samples=n_samples)
            if len(index_results) == 0:
                continue

            index_ids, index_scores = zip(*index_results)
            # combining ids and scores to list of lists
            ids.append(index_ids)
            scores.append(index_scores)

        ids, scores = CompositeIndex.flatten_column_wise(ids), CompositeIndex.flatten_column_wise(scores)

        if n_samples is None:
            n_samples = len(ids)

        idx = CompositeIndex.first_n_unique_ids(array=ids, n=n_samples)

        return [(ids[i], scores[i]) for i in idx]

    def estimate_preprocessed(self, preprocessed_query):
        results = self.search_preprocessed(preprocessed_query)
        results = sorted(results, key=lambda r: r[0])
        _, scores = zip(*results)
        return scores

    @staticmethod
    def flatten_column_wise(lists):
        """
        Flatten a list of lists in column-wise manner. Supports lists of unequal sizes.
        """
        elements_and_indices = []
        for i, lst in enumerate(lists):
            for j, el in enumerate(lst):
                elements_and_indices.append((el, i, j))  # adding column and row indices to the
        # Sorting lists so column-wise index is sorted first and then the row index
        elements_and_indices = sorted(elements_and_indices, key=lambda x: (x[2], x[1]))
        return [el_and_indices[0] for el_and_indices in elements_and_indices]

    @staticmethod
    def first_n_unique_ids(array, n):
        added_elements = set()
        first_ids = []
        for i, el in enumerate(array):
            if el not in added_elements:
                added_elements.add(el)
                first_ids.append(i)
            if len(first_ids) == n:
                break

        return first_ids

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        indexes = [indexes_map[index_name] for index_name in index_config['reuse_indexes']]
        return CompositeIndex(indexes)

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        indexes = [indexes_map[index_name] for index_name in index_config['reuse_indexes']]
        skill_to_index_map = dict()
        for skill_id, idxs in skill_to_idxs_map.items():
            skill_to_idxs_map[skill_id] = CompositeIndex([index[skill_id] for index in indexes])
        return skill_to_index_map
