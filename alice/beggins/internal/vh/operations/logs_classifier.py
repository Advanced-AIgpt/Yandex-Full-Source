import logging
from collections import Counter
from copy import deepcopy
from dataclasses import dataclass
from typing import Literal

import numpy as np
from sklearn.base import BaseEstimator
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.linear_model import SGDClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.preprocessing import Normalizer

from scipy import sparse
from scipy.stats import entropy as clc_entropy

import yt.wrapper as yt
from yt.wrapper.schema import TableSchema
import yandex.type_info.typing as ti

import vh3
from vh3 import MRTable, Enum, runtime, mr_run_base
from vh3.decorator import (
    operation, autorelease_to_nirvana_on_trunk_commit, nirvana_names
)

logger = logging.getLogger(__name__)
logging.basicConfig(level=logging.DEBUG)


class NGramProcessor:
    def __init__(self, is_bigram):
        self.vocab = {}
        self.col_counter = 0
        self._is_bigram = is_bigram

    def _get_tok_idf_stream(self, rows):
        for row in rows:
            def tok_iter(row):
                for tok, idf in row['idfs']:
                    tok = tok[0]
                    idf = idf[0]

                    if ' ' in tok and not self._is_bigram:
                        continue

                    yield tok, idf

            yield row['idfs'], tok_iter(row)

    def fit(self, rows):
        logger.debug('fit %s processor', 'bigram' if self._is_bigram else 'unigram')
        self.vocab = {}
        self.col_counter = 0
        for _, row_tokens in self._get_tok_idf_stream(rows):
            for tok, _ in row_tokens:
                if tok not in self.vocab:
                    self.vocab[tok] = self.col_counter
                    self.col_counter += 1

    def transform(self, rows):
        logger.debug('transform %s processor', 'bigram' if self._is_bigram else 'unigram')
        mrow = []
        mcol = []
        idfs_data = []
        ids_data = []
        rows_counter = 0

        for row_idfs, tokens in self._get_tok_idf_stream(rows):
            tok_counter = Counter(i[0][0] for i in row_idfs)
            for tok, idf in tokens:
                if tok not in self.vocab:
                    continue

                mrow.append(rows_counter)
                idfs_data.append(idf)
                ids_data.append(tok_counter[tok])
                mcol.append(self.vocab[tok])
            rows_counter += 1

        shape = (rows_counter, self.col_counter)

        ngrams = sparse.csr_matrix(
            (ids_data, (mrow, mcol)),
            shape=shape, dtype=int
        )

        idfs = sparse.csr_matrix(
            (idfs_data, (mrow, mcol)),
            shape=shape, dtype=float
        )

        # l2 norm
        norm = Normalizer()
        normalized_idfs = norm.transform(idfs)
        return ngrams, normalized_idfs


@dataclass
class ModelWrapper:
    classifier_name: str
    feature_name: str
    model: BaseEstimator

    def predict_proba(self, features):
        logger.debug('Applying model %s for features %s', self.classifier_name, self.feature_name)
        return self.model.predict_proba(
            features[self.feature_name]
        )


# uncertainty strategies
def full_entropy(votes):
    probs = np.hstack((votes, 1 - votes)) / len(votes)
    return clc_entropy(probs, base=2)


def vote_entropy(votes):
    n_estimators = len(votes)
    positive_votes = np.sum(votes >= 0.5) / n_estimators
    negative_votes = np.sum(votes < 0.5) / n_estimators
    probs = np.hstack((positive_votes, negative_votes))

    return clc_entropy(probs, base=2)


def kl_disagreement(votes):
    pos_votes = votes
    neg_votes = 1 - votes

    ones = np.ones(len(votes))
    avg_prob_pos = np.mean(pos_votes) * ones
    avg_prob_neg = np.mean(neg_votes) * ones

    probs = np.hstack((pos_votes, neg_votes))
    avgs = np.hstack((avg_prob_pos, avg_prob_neg))

    return clc_entropy(probs, avgs, base=2)


def consensus_entropy(votes):
    pos_votes = votes
    neg_votes = 1 - votes

    avg_prob_pos = np.mean(pos_votes)
    avg_prob_neg = np.mean(neg_votes)
    probs = np.hstack((avg_prob_pos, avg_prob_neg))

    return clc_entropy(probs, base=2)


disagreement_strategies = {
    'full_entropy': full_entropy,
    'vote_entropy': vote_entropy,
    'kl_disagreement': kl_disagreement,
    'consensus_entropy': consensus_entropy,
}


class MetaEstimator:
    """
    Query-By-Committee
    https://burrsettles.com/pub/settles.activelearning.pdf

    """

    def __init__(self, models):
        self.models = models

    def get_uncertainty(self, features, mode='full_entropy'):
        res = np.zeros((features['dssm'].shape[0], len(self.models)))
        models_count = len(self.models)

        for i, model in enumerate(self.models):
            res[:, i] = model.predict_proba(features)[:, 1]  # target == 1

        # calc disagreement
        uncertainty = disagreement_strategies[mode]
        unc_res = np.apply_along_axis(uncertainty, axis=1, arr=res)
        probas = np.sum(res, axis=1) / models_count  # bagging style

        return (unc_res, probas)


def chunker(seq, batch_size):
    assert batch_size > 0
    i = 0
    res = []
    seq = iter(seq)

    while True:
        if i < batch_size:
            try:
                res.append(next(seq))
                i += 1
            except StopIteration:
                break
        else:
            yield res
            res = []
            i = 0

    if res:
        yield res


class FeatureExtractor:
    def __init__(self):
        self.unigram = NGramProcessor(is_bigram=False)
        self.bigram = NGramProcessor(is_bigram=True)

    def fit(self, rows):
        logger.debug('Train feature extractor on %s rows', len(rows))
        target = []

        self.unigram.fit(rows)
        self.bigram.fit(rows)

        for row in rows:
            target.append(row['target'])

        return target

    def transform(self, rows):
        logger.debug('Apply feature extractor to %s rows', len(rows))
        dssm = []

        for row in rows:
            dssm.append(row['sentence_embedding'])

        unigrams, _ = self.unigram.transform(rows)
        bigrams, idf = self.bigram.transform(rows)

        features = {
            'unigram': unigrams,
            'ngram': bigrams,
            'idf': idf,
            'dssm': np.asarray(dssm, dtype=float),
        }

        return features


class Mapper:
    def __init__(
            self,
            meta_clf: MetaEstimator,
            feature_extractor: FeatureExtractor,
            batch_size: int,
            disagreement_metric: str,
    ):
        self._clf = meta_clf
        self._fe = feature_extractor
        self._batch_size = batch_size
        self._disagreement_metric = disagreement_metric

    @yt.aggregator
    def map(self, rows):
        for i, batch in enumerate(chunker(rows, self._batch_size)):
            logger.debug('Processing features for batch %s', i)
            features = self._fe.transform(batch)
            logger.debug('Calc uncertainties for batch %s', i)
            uncertainties, probabilities = self._clf.get_uncertainty(features, self._disagreement_metric)

            for row, unc, prob in zip(batch, uncertainties, probabilities):
                row['uncertainty'] = unc
                row['probability'] = prob
                yield row


def train_classifiers(train):
    train = list(train)

    algorithm_set = {
        'logreg': SGDClassifier(loss='log', alpha=1e-4, penalty='elasticnet'),
        'mlp': MLPClassifier(learning_rate_init=0.01),
        'rndf': RandomForestClassifier(n_estimators=100),
        'adaboost': GradientBoostingClassifier(loss='exponential'),
    }

    fe = FeatureExtractor()
    target = fe.fit(train)
    features_sets = fe.transform(train)

    # see https://st.yandex-team.ru/MEGAMIND-3384#620a7226329be10447339033
    combinations = [
        ('logreg', 'dssm'),
        ('logreg', 'idf'),
        ('mlp', 'idf'),
        ('rndf', 'ngram'),
        ('rndf', 'unigram'),
        ('adaboost', 'unigram'),
        ('adaboost', 'idf'),
    ]

    res = []
    for clf_name, feature_name in combinations:
        logger.info('Starting training %s model on %s features', clf_name, feature_name)
        model = deepcopy(algorithm_set[clf_name])
        features = features_sets[feature_name]
        model.fit(features, target)
        res.append(ModelWrapper(clf_name, feature_name, model))

    return MetaEstimator(res), fe


@operation(
    mr_run_base,
    owner='robot-voiceint',
    deterministic=True,
)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/alice_entropy_miner_inner/0.1.5',
    ya_make_folder_path='alice/beggins/internal/vh/cmd/logs_classifier',
)
@nirvana_names(mr_output_ttl='mr-output-ttl')
def alice_entropy_miner_inner(
    train_table: MRTable[yt.TablePath],
    logs_table: MRTable[yt.TablePath],
    disagreement_metric: Enum[
        Literal[
            'full_entropy',
            'vote_entropy',
            'kl_disagreement',
            'consensus_entropy',
        ]
    ] = 'full_entropy',
    max_ram: vh3.Integer = 16 * 1024,
    mr_output_ttl: vh3.Integer = 14,
) -> MRTable:
    """Select utterances that best suits for creating binary classifier

    Кубик принимает обучающую выборку и логи для грепа и ищет лучшие
    запросы которые нужно разметить в толоке для того чтобы потом
    обучать бинарный классификатор. На выходе 2 дополнительные
    колонки:
     - uncertainty чем больше тем больше классификатор не
    уверены к какому классу отнести запрос
     - probability чем выше тем
    больше уверенности у классификатора что пример positive

    """
    logger.debug('Get train table')
    train = list(yt.read_table(train_table))
    output_table = runtime.get_mr_output_path()
    schema = TableSchema.from_yson_type(yt.get(logs_table + "/@schema"))
    schema.add_column('uncertainty', ti.Double)
    schema.add_column('probability', ti.Double)

    logger.debug('Train classifiers')
    clf, fe = train_classifiers(train)

    logger.debug('Run mapper')
    yt.run_map(
        Mapper(clf, fe, batch_size=1024, disagreement_metric=disagreement_metric).map,
        logs_table,
        yt.TablePath(output_table, schema=schema),
        spec={"data_size_per_job": 80 * 1024 * 1024},
    )
