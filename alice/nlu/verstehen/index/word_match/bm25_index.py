import cPickle as pickle
import logging
import math
from collections import defaultdict

import numpy as np

from verstehen.index.index_registry import registered_index
from verstehen.preprocess import text_to_sequence, lemmatize_token
from verstehen.util import top_k_sorted_ids
from ..index import Index

logger = logging.getLogger(__name__)


@registered_index
class BM25Index(Index):
    """
    The index that searches the lemmatized queries in the lemmatized index.

    When search is performed, each token from each query part is looked up in all index
    entries. If the token is found in the index entry, entry's score is increased according
    to a score mapping that can depend on the token and the entry itself which is up to the
    implementation of the index. The process is repeated for each token in each query part
    (with rescoring entries) and then the best entries are returned.

    The rule for this index is based on the Okapi BM25 ranking function (see
    https://en.wikipedia.org/wiki/Okapi_BM25) with the exception that, if `use_query_freq`
    parameter is supplied with True value, then frequency term used in the original version
    is calculated based on a query and not the index entry.
    The reason behind this is based on the idea that our data is prone to have small index
    entries while query (which consists of many query parts) may be significantly bigger.
    Therefore, see, how frequent a particular token in the query parts (see
    `self._create_query_info`) and therefore it adjusts the scores of this token.
    """

    DEFAULT_CONFIG = {
        'index_type': 'bm25',

        # Okapi BM25 hyperparameters
        'k': 0.1,
        'b': 0.75,

        # TODO
        'use_query_freq': True,

        'index_path': None
    }

    def __init__(self, token_id_mapping, doc_lens, k=2.0, b=0.75, idf=None, use_query_freq=True):
        """
        Arguments:
            token_id_mapping: dict where keys are lemmatized tokens and values are a tuple
                where first value is a numpy array of indices of documents where this token
                occurred and the second value is a numpy array of a number of occurrences
                of this token, corresponding to ids in the first numpy array.
                NOTE: It is have numpy arrays to be sorted by the first array values (i.e.
                by ids of occurrences since it speeds up indexing).
            doc_lens: a single dimensional numpy array or a list of length of documents in
                the index.
            k: k parameter of the algorithm (see https://en.wikipedia.org/wiki/Okapi_BM25)
            b: b parameter of the algorithm (see https://en.wikipedia.org/wiki/Okapi_BM25)
            idf: an optional dict instance that represents idf scores for lemmatized strings.
                If value is absent in the map, then 1.0 is used as a default IDF value.
            use_query_freq: whether to use frequency term in queries alternatively to the
                initial frequencies in the documents (see class description for information).
        """

        self.token_id_mapping = token_id_mapping
        self.doc_lens = doc_lens
        if not isinstance(self.doc_lens, np.ndarray):
            self.doc_lens = np.array(self.doc_lens)
        self.avg_len = np.mean(doc_lens)

        self.bm25_k = k
        self.bm25_b = b
        self.use_query_freq = use_query_freq
        self.idf = idf if idf is not None else self.init_idf(doc_lens.shape[0])

        self.doc_lens_div_avg = self.doc_lens / self.avg_len

    def preprocessing(self, query):
        if isinstance(query, dict):
            query = query['positive']

        if not isinstance(query, (list, tuple, set)):
            query = [query]
        preprocessed_query = [BM25Index.preprocess_text(
            query_part) for query_part in query]
        return preprocessed_query

    def search_preprocessed(self, preprocessed_query, n_samples=None):
        if len(preprocessed_query) == 0:
            raise ValueError('Query must not be empty')

        scores_array = self.estimate_preprocessed(preprocessed_query)

        if n_samples is None or n_samples > scores_array.shape[0]:
            n_samples = scores_array.shape[0]

        # obtaining sorted ids for our predictions
        sorted_idx = top_k_sorted_ids(scores_array, n_samples)

        # final scores in the sorted manner
        sorted_scores = scores_array[sorted_idx]

        # remaining only scores that are higher than 0, otherwise they are meaningless
        return list(zip(sorted_idx[sorted_scores > 0], sorted_scores[sorted_scores > 0]))

    def estimate_preprocessed(self, preprocessed_query):
        query_info = self._create_query_info(preprocessed_query)

        new_preprocessed_query = set()
        for query_part in preprocessed_query:
            for token in query_part:
                new_preprocessed_query.add(token)

        scores_array = np.zeros(shape=(len(self.doc_lens),))

        for token in new_preprocessed_query:
            # skipping tokens that are not in our mapping
            if token not in self.token_id_mapping:
                continue

            # a map of where keys are ids of index entries where the token is found and
            # the values are number of occurrences of the token
            found_ids_map = self.token_id_mapping[token]

            # updating current scores_array given new found ids in index entries considering
            # what token we are looking at and the query general information
            self._update_scores_array(
                scores_array, found_ids_map, token, query_info)

        return scores_array

    def init_idf(self, total_docs):
        """
        Calculating IDF from self.token_id_mapping
        """
        idf = dict()
        for token, occurrences in self.token_id_mapping.items():
            idf[token] = math.log(
                (float(total_docs) + 1.0) / (len(occurrences) + 1.0))
        return idf

    def _update_scores_array(self, scores_array, found_ids_map, token, query_info):
        idf = self.idf[token]
        ids, occurrences = found_ids_map

        # we use freq term either the number of entries of the token in the whole query (`query_info[token]`)
        # or conventional number of occurrences of the token in the index entry divided by its length
        freq = query_info[token] if self.use_query_freq else \
            occurrences.astype(np.float32) / np.take(self.doc_lens, ids)

        # up term of for BM25 formula
        up_term = freq * (self.bm25_k + 1.0)

        # taking current lens divided by avg len for given ids
        cur_lens = np.take(self.doc_lens_div_avg, ids)

        # down term for BM25 formula
        down_term = freq + self.bm25_k * \
            (1.0 - self.bm25_b + self.bm25_b * cur_lens)

        # final calculation of the formula. We multiply the final value by additional `query_info[token]`
        # since we process only distinct tokens from the whole query instead of processing the same ones
        # many times. So, since we only add scores to each other, instead of processing same tokens we
        # only process distinct ones and multiply them by the number of their occurrences.
        scores_array[ids] += query_info[token] * idf * up_term / down_term

    def _create_query_info(self, query):
        """
        Creating the query information that is used in self._update_scores_array.

        The information is a dict where keys are tokens and values are frequency
        rates of the token being in different query parts.

        So if a token 'car' is seen twice in query parts and the total sum of
        lengths of query parts is 10 then the token will receive 0.2 score.
        """
        token_freq_info_map = defaultdict(float)

        sum_lens = 0
        for query_part in query:
            query_part = set(query_part)
            sum_lens += len(query_part)
            for token in query_part:
                token_freq_info_map[token] += 1.

        for token in token_freq_info_map:
            token_freq_info_map[token] /= sum_lens

        return token_freq_info_map

    @staticmethod
    def preprocess_text(text):
        sequence = text_to_sequence(text)
        lemmatized_sequence = [lemmatize_token(token) for token in sequence]
        return lemmatized_sequence

    @staticmethod
    def efficient_token_id_mapping(token_id_mapping):
        logger.debug(
            'Creating new BM25 mapping with sorted NumPy arrays for performance')

        efficient_token_id_mapping = dict()
        for token, occurrences in token_id_mapping.items():
            ids, number_of_occurrences = occurrences.keys(), occurrences.values()
            ids, number_of_occurrences = np.array(
                ids), np.array(number_of_occurrences)
            sorted_idx = np.argsort(ids)
            ids, number_of_occurrences = ids[sorted_idx], number_of_occurrences[sorted_idx]
            efficient_token_id_mapping[token] = (ids, number_of_occurrences)
        return efficient_token_id_mapping

    @staticmethod
    def from_texts(texts, k=2.0, b=0.75, idf=None, use_query_freq=True):
        logger.debug('Creating index {} from {} texts'.format(
            BM25Index.__name__, len(texts)))

        sequences = [BM25Index.preprocess_text(text) for text in texts]

        doc_lens = np.array([len(sequence) for sequence in sequences])

        token_id_mapping = defaultdict(lambda: defaultdict(int))
        for i, sequence in enumerate(sequences):
            for token in sequence:
                token_id_mapping[token][i] += 1

        # transforming id_mapping to numpy-powered structure
        token_id_mapping = BM25Index.efficient_token_id_mapping(
            token_id_mapping)

        return BM25Index(token_id_mapping, doc_lens=doc_lens, k=k, b=b, idf=idf, use_query_freq=use_query_freq)

    @staticmethod
    def from_bm25_mapping_file(path, k=2.0, b=0.75, idf=None, use_query_freq=True):
        logger.debug('Creating index {} from mapping by path {}'.format(
            BM25Index.__name__, path))

        with open(path, 'rb') as f:
            bm25_object = pickle.load(f)
        logger.debug('Pickle was read from {}'.format(path))

        token_id_mapping, doc_lens = bm25_object['token_id_mapping'], bm25_object['doc_lens']

        return BM25Index(token_id_mapping, doc_lens=doc_lens, k=k, b=b, idf=idf, use_query_freq=use_query_freq)

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        k = index_config['k']
        b = index_config['b']
        use_query_freq = index_config['use_query_freq']

        path = index_config.get('index_path', None)
        if path is not None:
            index = BM25Index.from_bm25_mapping_file(
                path, k=k, b=b, use_query_freq=use_query_freq)
        else:
            index = BM25Index.from_texts(
                texts, k=k, b=b, use_query_freq=use_query_freq)
        return index
