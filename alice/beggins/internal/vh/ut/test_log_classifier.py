import json
import pytest
import numpy as np
from sklearn.preprocessing import Normalizer
from alice.beggins.internal.vh.operations import logs_classifier as lc

import yatest.common as yc


@pytest.fixture
def default_rows():
    return [
        {
            'idfs': [
                [["алиса"], [2.1812575851665654]],
                [["алиса озвучь"], [12.608270548070125]],
                [["на странице"], [9.901861434726557]],
                [["текст"], [7.758304977837734]],
                [["текст на"], [10.792591424358095]],
                [["озвучь"], [11.148616982484732]],
                [["озвучь текст"], [14.108353604229386]],
                [["странице"], [9.60274069946438]]
            ],
            'sentence_embedding': [1, 2, 3],
            'text': 'Алиса озвучь текст на странице',
            'target': 1,
        },
        {
            'idfs': [
                [["алиса"], [2.1812575851665654]],
                [["алиса когда"], [7.5824749573775225]],
                [["рамадан"], [12.950025232148295]],
                [["начинается"], [8.39466385679029]],
                [["начинается рамадан"], [16.66421198998967]],
                [["когда начинается"], [11.056434614409419]]
            ],
            'sentence_embedding': [1, 2, 3],
            'text': 'алиса когда начинается рамадан',
            'target': 0,
        },
    ]


@pytest.fixture
def train_data():
    res = []
    with open(yc.work_path('MEGAMIND_3384_test_data_train')) as f:
        for line in f:
            res.append(json.loads(line))

    return res


@pytest.fixture
def probabilities():
    # rows - samples
    # columns - estimators
    # values - probability of sample to be positive
    return np.asarray([
        [0.1, 0.1, 0.3, 0.7],
        [0.5, 0.7, 0.2, 0.4],
        [0.9, 0.9, 0.7, 0.3],
        [0.5, 0.7, 0.3, 0.6],
        [0.9, 0.3, 0.7, 0.3],
        [0.1, 0.4, 0.4, 0.5],
    ])


def test_unigram(default_rows):
    train = [default_rows[0]]
    unigram = lc.NGramProcessor(is_bigram=False)
    unigram.fit(train)

    assert unigram.vocab == {'алиса': 0, 'текст': 1, 'озвучь': 2, 'странице': 3}
    assert unigram.col_counter == 4

    ngrams, idfs_data = unigram.transform(train)

    norm = Normalizer()

    res = [2.1812575851665654, 7.758304977837734,
           11.148616982484732, 9.60274069946438]
    assert np.all(np.isclose(
        idfs_data.todense(),
        norm.transform([res])
    ))
    assert np.array_equal(
        ngrams.todense(),
        [[1, 1, 1, 1]]
    )


def test_bigram(default_rows):
    train = [default_rows[1]]
    bigram = lc.NGramProcessor(is_bigram=True)
    bigram.fit(train)
    assert bigram.vocab == {
        'алиса': 0, 'алиса когда': 1, 'рамадан': 2, 'начинается': 3,
        'начинается рамадан': 4, 'когда начинается': 5
    }
    assert bigram.col_counter == 6


def test_clc_features(default_rows):
    fe = lc.FeatureExtractor()
    target = fe.fit(default_rows)
    features = fe.transform(default_rows)
    assert target == [1, 0]

    assert np.array_equal(
        features['dssm'],
        np.asarray([
            [1, 2, 3],
            [1, 2, 3],
        ], dtype=float)
    )
    assert np.array_equal(
        features['unigram'].todense(),
        np.asarray([
            [1, 1, 1, 1, 0, 0],
            [1, 0, 0, 0, 1, 1],
        ]),
    )

    assert np.array_equal(
        features['ngram'].todense(),
        np.asarray([
            [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0],
            [1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1],
        ]),
    )

    assert np.array_equal(
        features['idf'].todense() > 0,
        features['ngram'].todense() > 0,
    )


@pytest.mark.parametrize('seq,batch,out', [
    ([], 1, []),
    ([], 100, []),
    ([1, 2, 3], 2, [[1, 2], [3]]),
    (range(10), 3, [[0, 1, 2], [3, 4, 5], [6, 7, 8], [9]]),
])
def test_chunker(seq, batch, out):
    assert list(lc.chunker(seq, batch)) == out


def test_model_training(train_data):
    train = train_data
    clf, fe = lc.train_classifiers(train)

    features = fe.transform(train)
    uncertainties, probabilities = clf.get_uncertainty(features)
    assert list(map(lambda x: int(x), probabilities > 0.5)) == [t['target'] for t in train]


def test_mapper(train_data):
    clf, fe = lc.train_classifiers(train_data)
    res = list(
        lc.Mapper(clf, fe, batch_size=10, disagreement_metric='full_entropy').map(train_data)
    )

    features = fe.transform(train_data)
    uncertainties, probabilities = clf.get_uncertainty(features)

    assert np.all(np.isclose(
        uncertainties,
        [i['uncertainty'] for i in res]
    ))
    assert np.all(np.isclose(
        probabilities,
        [i['probability'] for i in res]
    ))


def test_full_entropy(probabilities):
    assert np.all(np.isclose(
        np.apply_along_axis(lc.full_entropy, axis=1, arr=probabilities),
        [2.67514325, 2.8935424, 2.67514325, 2.9333831, 2.77821707, 2.8527242]
    ))


def test_vote_entropy(probabilities):
    assert np.all(np.isclose(
        np.apply_along_axis(lc.vote_entropy, axis=1, arr=probabilities),
        [0.81127812, 1.0, 0.81127812, 0.81127812, 1.0, 0.81127812]
    ))


def test_kl_disagreement(probabilities):
    assert np.all(np.isclose(
        np.apply_along_axis(lc.kl_disagreement, axis=1, arr=probabilities),
        [0.20614765, 0.09923206, 0.20614765, 0.06481278, 0.21455738, 0.08134386]
    ))
