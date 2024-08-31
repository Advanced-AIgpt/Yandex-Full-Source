# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import numpy as np

from itertools import izip
from collections import Mapping
from vins_core.nlu.features.extractor.base import BaseFeatureExtractor, DenseFeatures, DenseSeqFeatures
from query_wizard_features_reader import QueryWizardFeaturesReader
from vins_core.utils.data import get_resource_full_path, load_data_from_file


logger = logging.getLogger(__name__)


class SerpFeatureExtractor(BaseFeatureExtractor):
    """Class which extracts features with statistics of query over different verticals.

    Parameters:
        - trie_path (str)- trie file for query features
        - data_path (str)- data file for query features
        - sequence (bool) - whether to return "local" features, different for each token,
            or "global" features as non-sequential - once per query
        - frequency_file_or_dict (str or Mapping or None) - parameters for IDF word weighting.
            If it is a Mapping, it should contain numeric value 'bias' and Mapping 'counts' with numeric values.
            If it is a string, it should be the link to a JSON file containing such a mapping
        - prior_strength (numeric) - amount of smoothing that applies to all wizard frequencies and surpluses
        - log_frequency (bool) - whether to log-transform wizard frequencies before averaging
        - log_surplus (bool) - whether to logit-transform the mean surpluses before averaging

    For example, for query "скажи мне погоду на завтра пожалуйста", QueryWizardFeaturesReader returns items like this:

    [{
        "fragment":"погода на завтра",
        "normalized_fragment":"погода на завтра",
        "start":10,
        "length":16,
        "features":[4.5,0.9375,0,0.3125,0,0.9375,0.125,0,0,0,0,0,0,0,0,0,0]
    },
    {
        "fragment":"погода",
        "normalized_fragment":"погода",
        "start":10,
        "length":6,
        "features":[....]}, ......<other substrings of tokens>.....]

    for different substrings of original query, which are popular queries.

    Features explanation:
        0. Avg number of users per day which types this query (we don't use it).
        1-2. pair of numbers (% of such queries which were shown with wizard of Images vertical, surplus of this query)
        3-4. the same for Video
        5-6. Weather
        7-8. Music
        9-10. Factoid (object answer)
        11-12. Organizations
        13-14. Maps
        15-16. Navi
        17-18. Market

    We do not consider 0 feature (avg users).

    'Global' TokenFeatures (if `sequence` == False) contain:
        * maximum from all raw features over all subqueries
        * mean raw features over all subqueries
        * mean raw features over all subqueries weighted by "responsibility" for query words and popularity (feature 0)
        * boolean feature (longest subquery == utterance)
        * inverse difference between length of longest subquery and utterance.
    This should add up to 56 features in total

    'Local' TokenFeatures (if `sequence` == True) contain only mean raw features - 18 in total.
    For each word, they are averaged only over the subqueries containing this word.
    """

    _USED_FEATURES_RANGE = (1, 19)  # We drop first feature "avg number of users".
    _USED_FEATURES_SLICE = slice(*_USED_FEATURES_RANGE)

    _RAW_FEATURE_NAMES = ('avg_images', 'surplus_images', 'avg_videos', 'surplus_videos',
                          'avg_weather', 'surplus_weather', 'avg_music', 'surplus_music',
                          'avg_factoid', 'surplus_factoid', 'avg_org', 'surplus_org',
                          'avg_maps', 'surplus_maps', 'avg_navi', 'surplus_navi', 'avg_market', 'surplus_market')

    # these frequencies were estimated as averages over the february validation bin for each wizard
    _MEAN_FREQUENCIES = (0.6, 0.6, 0.014, 0.1, 0.12, 0.06, 0.05, 0.05, 0.03)

    _MIN_SURPLUS_FOR_LOGIT = -0.999
    _MAX_SURPLUS_FOR_LOGIT = 0.999

    SEQ_FEATURE_NAMES = tuple('token_wmean__' + fname for fname in _RAW_FEATURE_NAMES)
    FEATURE_NAMES = (tuple('max__' + fname for fname in _RAW_FEATURE_NAMES) +
                     tuple('mean__' + fname for fname in _RAW_FEATURE_NAMES) +
                     tuple('wmean__' + fname for fname in _RAW_FEATURE_NAMES) +
                     tuple(('longest_is_utterance', 'invlendiff'))
                     )

    def __init__(self, trie, data, sequence=True,
                 frequency_file_or_dict=None, prior_strength=0, log_frequency=False, log_surplus=False, **kwargs):
        super(SerpFeatureExtractor, self).__init__()

        if data is None or trie is None:
            raise ValueError('Query wizard features resource cannot be empty')

        trie_path, data_path = self._load_resource(trie, data)

        self._query_wizard_features_reader = QueryWizardFeaturesReader(trie_path, data_path)
        self._sequence = sequence
        self._info = self.SEQ_FEATURE_NAMES if self._sequence else self.FEATURE_NAMES
        if frequency_file_or_dict is not None:
            if isinstance(frequency_file_or_dict, Mapping):
                wordcount = frequency_file_or_dict
            else:
                wordcount = load_data_from_file(frequency_file_or_dict)
            self._word_frequency = wordcount['counts']
            self._word_frequency_bias = wordcount['bias']
        else:
            self._word_frequency = dict()
            self._word_frequency_bias = 1
        self._prior_strength = prior_strength
        self._log_frequency = log_frequency
        self._log_surplus = log_surplus

    @classmethod
    def _serp_input(cls, sample):
        return sample.text

    def _parse_serp_result(self, sample, res):
        serp_input = self._serp_input(sample)

        all_items_features = np.array([item['features'] for item in res])

        if all_items_features.shape[1] < self._USED_FEATURES_RANGE[1]:
            logger.warning("The number of returned features by Serp is less than expected! Feature shift has occured.")

        items_features = all_items_features[:, self._USED_FEATURES_SLICE]

        item_popularity = all_items_features[:, 0]
        query_tokens = serp_input.split()
        subqueries_texts = [item['fragment'] for item in res]
        alignment = self._get_alignment(serp_input, subqueries_texts)
        if self._word_frequency:
            token_weights = np.array([1.0 / (self._word_frequency.get(token, 0) + self._word_frequency_bias)
                                      for token in query_tokens])
        else:
            token_weights = None
        subquery_responsibility = (alignment / alignment.sum(axis=0))
        # there might be words that are not contained in any subquery
        subquery_responsibility[np.isnan(subquery_responsibility)] = 0
        return serp_input, items_features, item_popularity, subquery_responsibility, token_weights

    def normalize_features(self, items_features, item_popularity):
        if not (self._prior_strength or self._log_frequency or self._log_surplus):
            return items_features
        m = items_features.shape[1]
        new_features = np.zeros(items_features.shape)
        # normalize frequencies: apply prior and take logs
        # for each wizard, its frequency is smoothed towards unconditional mean, stored in _MEAN_FREQUENCIES
        # _prior_strength regulates the amount of smoothing: 0 is no smoothing, 1000 is quite a heavy smoothing
        for i in range(0, m, 2):
            shown = items_features[:, i] * item_popularity + self._MEAN_FREQUENCIES[i//2] * self._prior_strength
            total = item_popularity + self._prior_strength
            if self._log_frequency:
                new_features[:, i] = np.log(shown / total)
            else:
                new_features[:, i] = shown / total
        # normalize surpluses: apply prior and take tanh
        # for each wizard, its surplus is smoothed towards unconditional mode, equal to 0
        # _prior_strength regulates the amount of smoothing: 0 is no smoothing, 1000 is quite a heavy smoothing
        for i in range(1, m, 2):
            total_surplus = items_features[:, i] * item_popularity
            total_obs = item_popularity + self._prior_strength
            if self._log_surplus:
                surplus = np.minimum(self._MAX_SURPLUS_FOR_LOGIT, np.maximum(self._MIN_SURPLUS_FOR_LOGIT,
                                                                             total_surplus / total_obs))
                new_features[:, i] = np.log((1 + surplus) / (1 - surplus))
            else:
                new_features[:, i] = total_surplus / total_obs
        return new_features

    def _get_sequential_features(self, sample, res):
        """ Calculate matrix token * feature, with features weighted across the subqueries containing the token """
        serp_input, items_features, item_popularity, subquery_responsibility, token_weights \
            = self._parse_serp_result(sample, res)
        weighted_responsibility = subquery_responsibility.T * item_popularity
        weighted_responsibility = (weighted_responsibility.T / weighted_responsibility.sum(axis=1)).T
        weighted_responsibility[np.isnan(weighted_responsibility)] = 0
        word_features = np.dot(weighted_responsibility, self.normalize_features(items_features, item_popularity))
        return word_features

    def _get_features_list(self, sample, res):
        """For each token return a list of features from the response which contain this token.
        """
        serp_input, items_features, item_popularity, subquery_responsibility, token_weights \
            = self._parse_serp_result(sample, res)

        # Max, mean features across subqueries.
        max_features = np.max(items_features, axis=0)  # 16 numbers
        mean_features = np.mean(items_features, axis=0)

        # weighted mean
        if token_weights is not None:
            subquery_responsibility = subquery_responsibility * token_weights
        item_weights = subquery_responsibility.sum(axis=1) * item_popularity
        sum_of_weights = item_weights.sum()
        if sum_of_weights > 0:
            item_weights /= sum_of_weights

        weighted_mean_features = (self.normalize_features(items_features, item_popularity).T * item_weights).sum(axis=1)

        # Longest features
        # Finding max len item
        def item_length(item):
            start_pos = item['start']
            length = item['length']
            length_in_tokens = len(serp_input[start_pos:start_pos + length].split())
            max_feature = max(item['features'][self._USED_FEATURES_SLICE])
            return length_in_tokens, max_feature

        argmax = max(izip(map(item_length, res), xrange(len(res))))[1]

        diff = np.array([1./(abs(res[argmax]['length']-len(serp_input))+1)])
        longest_is_utterance = np.array([int(res[argmax]['length'] == len(serp_input))])

        return np.concatenate((
            max_features,
            mean_features,
            weighted_mean_features,
            longest_is_utterance,
            diff
        ))

    def _fallback_result(self, sample):
        result = []
        if self._sequence:
            result.append(DenseSeqFeatures(np.zeros((len(sample), len(self._RAW_FEATURE_NAMES)))))
        else:
            result.append(DenseFeatures(np.zeros(len(self.info))))
        return result

    def _call(self, sample, **kwargs):
        try:
            res = self._query_wizard_features_reader.get_features(self._serp_input(sample))
        except Exception:
            return self._fallback_result(sample)
        if not res:
            return self._fallback_result(sample)
        result = []
        if self._sequence:
            features = np.asarray(self._get_sequential_features(sample, res))
            result.append(DenseSeqFeatures(features))
        else:
            features = np.asarray(self._get_features_list(sample, res))
            result.append(DenseFeatures(features))
        return result

    @staticmethod
    def _get_alignment(query_text, subqueries_texts):
        """ Create a binary alignment matrix for a query and set of subqueries """
        alignment = np.zeros((len(subqueries_texts), query_text.count(" ") + 1))
        query_length = len(query_text)
        index2word = []
        n_words = 0
        for c in query_text:
            if c == " ":
                n_words += 1
            index2word.append(n_words)

        for i, subquery_text in enumerate(subqueries_texts):
            n_words = subquery_text.count(" ") + 1
            start = 0
            while True:
                pos = query_text.find(subquery_text, start)
                if pos == -1:
                    break
                start = pos + 1
                if pos > 0 and query_text[pos - 1] != " ":
                    continue
                sub_end = pos + len(subquery_text)
                if sub_end < query_length and query_text[sub_end] != " ":
                    continue
                first_word = index2word[pos]
                for j in range(n_words):
                    alignment[i, first_word + j] = 1

        return alignment

    def _load_resource(self, trie_resource_path, data_resource_path):
        logger.info("Load query_wizard_features resource from %s, %s", trie_resource_path, data_resource_path)
        trie_path = get_resource_full_path(trie_resource_path)
        data_path = get_resource_full_path(data_resource_path)

        return trie_path, data_path

    @property
    def _features_cls(self):
        return DenseSeqFeatures if self._sequence else DenseFeatures
