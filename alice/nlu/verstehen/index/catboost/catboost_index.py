import math
import random
from collections import defaultdict

import catboost as cb
import numpy as np
import sklearn

from verstehen.index import Index
from verstehen.index.embedding import DssmKnnIndex
from verstehen.index.index_registry import registered_index
from verstehen.index.word_match import BM25Index
from verstehen.util import top_k_sorted_ids, approximate_balanced_train_test_split


@registered_index
class CatBoostIndex(Index):
    """
    Index that performs search using CatBoost text features. Currently we support 3 types of text
    features that are derived from text:
        1. DSSM based features:
            - Distance to the average positive vector
            - Distance to the average negative vector
            - Distance to the closest positive vector
            - Distance to the closest negative vector
        2. Naive Bayes features:
            - Naive Bayes score of the text to be in positive class
            - Naive Bayes score of the text to be in negative class
            - Relation of Naive Bayes scores (positive / negative)
        3. Okapi BM25 features:
            - BM25 score of the text to be in positive class
            - BM25 score of the text to be in negative class

    The features are computed in `online` manner for the query (training data) and the accumulated
    statistics are used in the index (inference data).
    In other words, we traverse the received query and compute the features for each sample in the
    query while, in parallel, we update the statistics after each feature computation given that
    we know whether a sample is positive or not.
    For example, consider DSSM feature of distance to the average vectors. Given a sample, we
    calculate the distance to average positive and negative vectors, producing two features and
    only then we `look` into the label of the sample. If the sample turns out to be positive, we
    update the average positive vector, if it is negative we update the average negative vector.
    After updating these vectors we analyze the next sample in the same way as the previous one but
    we use already updated statistics to calculate the features.

    We also use some portion of the query, if available, to perform validation of the Catboost
    model in order to improve performance.

    NOTE: this index is not designed to be used for big indexes with millions of entries as the
    Catboost applier is not as fast to process so many data points in short time.
    """

    DEFAULT_CONFIG = {
        'index_type': 'catboost',

        # name of the index for DSSM index
        'reuse_dssm_index_name': None,
        # or create it from config
        'dssm_index_config': DssmKnnIndex.DEFAULT_CONFIG,

        'min_negative_query_samples': 50,

        'cb_depth': 6,
        'cb_lr': 0.3,
        'cb_num_trees': 200,
        'cb_thread_count': 20,
        'cb_val_portion': 0.2,
        'cb_early_stopping_rounds': 3
    }

    def __init__(self, index_texts, dssm_applier, index_embeddings, min_negative_query_samples,
                 cb_depth, cb_lr, cb_num_trees, cb_thread_count, cb_val_portion, cb_early_stopping_rounds):
        self.features_maker = OnlineFeaturesMaker(dssm_applier, min_negative_query_samples=min_negative_query_samples)
        self.index_data = IndexData(
            index_texts,
            index_embeddings,
            [BM25Index.preprocess_text(text) for text in index_texts]  # TODO remove dependency for another BM25Index
        )
        # CatBoost options
        self.cb_depth = cb_depth
        self.cb_lr = cb_lr
        self.cb_num_trees = cb_num_trees
        self.cb_thread_count = cb_thread_count
        self.cb_val_portion = cb_val_portion
        self.cb_early_stopping_rounds = cb_early_stopping_rounds

    def preprocessing(self, query):
        preprocessed_query = self.features_maker.make_features(query, self.index_data)
        return preprocessed_query

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        n_samples = min(n_samples, len(self.index_data.preprocessed_texts))
        scores = self.estimate_preprocessed(preprocessed_query)
        sorted_idx = top_k_sorted_ids(scores, n_samples)
        return list(zip(sorted_idx, scores[sorted_idx].astype(np.float64)))

    def estimate_preprocessed(self, preprocessed_query):
        X, y = preprocessed_query.query_features, np.array(preprocessed_query.query_target, dtype=np.int32)

        X_train, X_val, y_train, y_val = approximate_balanced_train_test_split(
            X, y, test_targets_proportion=self.cb_val_portion
        )

        classifier = cb.CatBoostClassifier(
            depth=self.cb_depth,
            learning_rate=self.cb_lr,
            num_trees=self.cb_num_trees,
            thread_count=self.cb_thread_count
        ).fit(
            X_train, y_train,
            eval_set=(X_val, y_val) if len(X_val) > 0 else None,
            use_best_model=True,
            early_stopping_rounds=self.cb_early_stopping_rounds
        )
        scores = classifier.predict_proba(preprocessed_query.index_features)[:, 1]
        return scores

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        min_negative_query_samples = index_config['min_negative_query_samples']
        cb_depth = index_config['cb_depth']
        cb_lr = index_config['cb_lr']
        cb_num_trees = index_config['cb_num_trees']
        cb_thread_count = index_config['cb_thread_count']
        cb_val_portion = index_config['cb_val_portion']
        cb_early_stopping_rounds = index_config['cb_early_stopping_rounds']

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

        return CatBoostIndex(
            texts, dssm_index.dssm_applier, dssm_index._get_index_embeddings(), min_negative_query_samples,
            cb_depth, cb_lr, cb_num_trees, cb_thread_count, cb_val_portion, cb_early_stopping_rounds
        )

    @staticmethod
    def from_config_by_skill_id(index_config, texts, payload=None, indexes_map=None, skill_to_idxs_map=None):
        min_negative_query_samples = index_config['min_negative_query_samples']
        cb_depth = index_config['cb_depth']
        cb_lr = index_config['cb_lr']
        cb_num_trees = index_config['cb_num_trees']
        cb_thread_count = index_config['cb_thread_count']
        cb_val_portion = index_config['cb_val_portion']
        cb_early_stopping_rounds = index_config['cb_early_stopping_rounds']

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

        skill_to_index_map = dict()
        for skill_id, idxs in skill_to_idxs_map.items():
            skill_to_index_map[skill_id] = CatBoostIndex(
                texts[skill_id], dssm_index.dssm_applier, dssm_index._get_index_embeddings(), min_negative_query_samples,
                cb_depth, cb_lr, cb_num_trees, cb_thread_count, cb_val_portion, cb_early_stopping_rounds
            )
        return skill_to_index_map


class OnlineFeaturesMaker:
    """
    Class to output online features of DSSM, Naive Bayes and Okapi BM25.
    """

    def __init__(self, dssm_applier, min_negative_query_samples):
        self.dssm_applier = dssm_applier
        self.min_negative_query_samples = min_negative_query_samples

    def make_features(self, query_data, index_data):
        # initializing algorithms for calculating features and accumulating statistics
        dssm = DSSMOnline()
        bm25 = BM25Online()
        naive_bayes = NaiveBayesOnline()

        # Adding negative samples to the query if necessary
        cur_negative = len(query_data['negative'])
        if cur_negative < self.min_negative_query_samples:
            sampled = random.sample(index_data.texts, k=self.min_negative_query_samples - cur_negative)
            query_data['negative'] = query_data['negative'] + sampled

        texts = query_data['positive'] + query_data['negative']
        labels = np.array([i < len(query_data['positive']) for i in range(len(texts))])

        # preprocessing query data for DSSM and BM25
        embeddings = np.array([self.dssm_applier.predict(text) for text in texts])
        # TODO remove dependency for another BM25Index
        preprocessed_texts = np.array([BM25Index.preprocess_text(text) for text in texts])

        # shuffling all data creating random permutation
        labels, embeddings, preprocessed_texts = sklearn.utils.shuffle(labels, embeddings, preprocessed_texts)

        for label, embedding, preprocessed_text in zip(labels, embeddings, preprocessed_texts):
            dssm.calc_features_and_add_query(label, embedding)
            naive_bayes.calc_features_and_add_query(label, preprocessed_text)
            bm25.calc_features_and_add_query(label, preprocessed_text)

        query_features = np.column_stack([
            dssm.negative_scores,
            dssm.positive_scores,
            dssm.negative_mean_scores,
            dssm.positive_mean_scores,
            bm25.negative_scores,
            bm25.positive_scores,
            naive_bayes.negative_scores,
            naive_bayes.positive_scores,
            naive_bayes.relative_scores
        ])

        dssm_features = dssm.calc_features_for_index(index_data.index_embeddings)
        bm25_features = bm25.calc_features_for_index(index_data.preprocessed_texts)
        naive_bayes_features = naive_bayes.calc_features_for_index(index_data.preprocessed_texts)
        all_features = np.hstack([dssm_features, bm25_features, naive_bayes_features])

        return PreprocessedQuery(query_features, labels, all_features)


class IndexData:
    def __init__(self, texts, index_embeddings, index_preprocessed_texts):
        self.texts = texts
        self.preprocessed_texts = index_preprocessed_texts
        self.index_embeddings = index_embeddings


class PreprocessedQuery:
    def __init__(self, query_features, query_target, index_features):
        self.query_features = query_features
        self.query_target = query_target
        self.index_features = index_features


class NaiveBayesOnline:
    def __init__(self):
        self.positive_tokens = defaultdict(int)
        self.negative_tokens = defaultdict(int)

        self.positive_len = 0
        self.negative_len = 0

        self.positive_cnt = 0
        self.negative_cnt = 0

        self.all_words_set = set()

        self.positive_scores = []
        self.negative_scores = []
        self.relative_scores = []

    def calc_features_and_add_query(self, class_id, query):
        self.positive_scores.append(self.calc_pscore(query))
        self.negative_scores.append(self.calc_nscore(query))
        if self.positive_scores[-1] != 0:
            self.relative_scores.append(self.negative_scores[-1] / self.positive_scores[-1])
        else:
            self.relative_scores.append(0)
        if class_id == 0:
            for t in query:
                self.negative_tokens[t] += 1
                self.all_words_set.add(t)
            self.negative_len += len(query)
            self.negative_cnt += 1
        else:
            for t in query:
                self.positive_tokens[t] += 1
                self.all_words_set.add(t)
            self.positive_len += len(query)
            self.positive_cnt += 1

    def calc_pscore(self, query):
        return self.calc_score(self.positive_tokens, self.positive_len, self.positive_cnt, query)

    def calc_nscore(self, query):
        return self.calc_score(self.negative_tokens, self.negative_len, self.negative_cnt, query)

    def calc_score(self, scores, class_len, class_phrase_cnt, query):
        score = sum([math.log(scores[t] + 1.0) for t in query])
        score -= len(query) * math.log(class_len + len(self.all_words_set) + 1.0)
        score += math.log(class_phrase_cnt + 1.0)
        return score

    def calc_features_for_index(self, preprocessed_texts):
        negative_scores, positive_scores = [], []
        for text in preprocessed_texts:
            negative_scores.append(self.calc_nscore(text))
            positive_scores.append(self.calc_pscore(text))

        relative_scores = [neg / pos if pos != 0 else 0 for neg, pos in zip(negative_scores, positive_scores)]

        return np.column_stack([
            np.array(negative_scores),
            np.array(positive_scores),
            np.array(relative_scores)
        ])


class BM25Online:
    def __init__(self):
        # TODO configure these parameters
        self.k1 = 2.0
        self.b = 0.75
        self.positive_tokens = defaultdict(int)
        self.negative_tokens = defaultdict(int)
        self.positive_token_count = 0
        self.negative_token_count = 0
        self.positive_scores = []
        self.negative_scores = []

    def calc_features_and_add_query(self, class_id, query):
        self.positive_scores.append(self.calc_pscore(query))
        self.negative_scores.append(self.calc_nscore(query))
        if class_id == 0:
            for t in query:
                self.negative_tokens[t] += 1
                self.negative_token_count += 1
        else:
            for t in query:
                self.positive_tokens[t] += 1
                self.positive_token_count += 1

    @staticmethod
    def preprocess_texts(texts):
        return [BM25Index.preprocess_text(t) for t in texts]

    def calc_pscore(self, query):
        return self.calc_score(self.positive_tokens, self.positive_token_count, query)

    def calc_nscore(self, query):
        return self.calc_score(self.negative_tokens, self.negative_token_count, query)

    def calc_score(self, score_terms, score_terms_cnt, query):
        """
        :param score_terms: class for which we calc scores
        :param score_terms_cnt: num of scores in this class
        :param query:
        :return: float score
        """
        tokens_count = self.positive_token_count + self.negative_token_count
        if tokens_count == 0:
            return 0
        score = 0.0
        for t in query:
            num_term_docs = 1 if t in self.positive_tokens else 0
            num_term_docs += 1 if t in self.negative_tokens else 0
            idf = math.log((2 - num_term_docs + 0.5) / (num_term_docs + 0.5))
            score += idf * ((score_terms[t] * (self.k1 + 1)) /
                            (score_terms[t] + self.k1 * (1 - self.b + self.b * score_terms_cnt * 2. / tokens_count)))
        return score

    def calc_features_for_index(self, preprocessed_texts):
        negative_scores, positive_scores = [], []

        for text in preprocessed_texts:
            negative_scores.append(self.calc_nscore(text))
            positive_scores.append(self.calc_pscore(text))

        return np.column_stack([
            np.array(negative_scores),
            np.array(positive_scores)
        ])


class DSSMOnline:
    # TODO: optimization #1, having average vector instead of all vectors to calculate mean distance
    # TODO: optimization #2, having hnsw instead of all vectors matmul to find the closest one

    def __init__(self):
        # embedding score class
        self.positive_embeddings = None
        self.negative_embeddings = None

        self.positive_scores = []
        self.negative_scores = []
        self.positive_mean_scores = []
        self.negative_mean_scores = []

    def calc_features_and_add_query(self, class_id, query):
        self.positive_scores.append(self.calc_pscore(query))
        self.negative_scores.append(self.calc_nscore(query))
        self.positive_mean_scores.append(self.calc_mean_pscore(query))
        self.negative_mean_scores.append(self.calc_mean_nscore(query))
        if class_id == 0:
            if self.negative_embeddings is not None:
                self.negative_embeddings = np.vstack((self.negative_embeddings, query))
            else:
                self.negative_embeddings = np.array([query])
        else:
            if self.positive_embeddings is not None:
                self.positive_embeddings = np.vstack((self.positive_embeddings, query))
            else:
                self.positive_embeddings = np.array([query])

    def calc_pscore(self, query):
        return self.calc_score(self.positive_embeddings, query)

    def calc_nscore(self, query):
        return self.calc_score(self.negative_embeddings, query)

    def calc_mean_pscore(self, query):
        return self.calc_score(self.positive_embeddings, query, np.mean)

    def calc_mean_nscore(self, query):
        return self.calc_score(self.negative_embeddings, query, np.mean)

    def calc_score(self, embeddings, query, filter_func=np.max):
        """
        :param embeddings: matrix n * embedding_size
        :param query: vector of embedding_size elements
        :param filter_func:
        :return: float score
        """
        if embeddings is None:
            return -2
        return filter_func(np.matmul(embeddings, np.expand_dims(query, axis=1)))

    def calc_features_for_index(self, index_embeddings):
        pos_emb_t = self.positive_embeddings.T
        neg_emb_t = self.negative_embeddings.T

        return np.column_stack([
            np.max(np.matmul(index_embeddings, neg_emb_t), axis=1),
            np.max(np.matmul(index_embeddings, pos_emb_t), axis=1),
            np.mean(np.matmul(index_embeddings, neg_emb_t), axis=1),
            np.mean(np.matmul(index_embeddings, pos_emb_t), axis=1)
        ])
