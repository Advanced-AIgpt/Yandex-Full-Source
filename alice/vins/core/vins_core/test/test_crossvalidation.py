# coding: utf-8
from __future__ import unicode_literals

import pytest
import numpy as np
import string
import vins_core

from itertools import izip
from collections import OrderedDict

from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.nlu.token_tagger import CRFTokenTagger
from vins_core.test.test_data.nlu_eval_data import weather_data
from vins_core.nlu.features_extractor import create_features_extractor
from vins_core.nlu.token_classifier import create_token_classifier

rng = np.random.RandomState(42)  # for running outside test suites


def toy_data(forward):
    tags = string.ascii_lowercase
    toks = [unicode(i) for i in xrange(len(tags))]
    trm = {}
    for prev_tag in tags:
        trm[prev_tag, '0'] = prev_tag
        for curr_tag, curr_tok in izip(
            filter(lambda t: t != prev_tag, tags),
            filter(lambda t: t != '0', toks)
        ):
            trm[prev_tag, curr_tok] = curr_tag
    prior = dict(zip(toks, tags))
    num_examples = 1000
    example_len = 10
    seqs = np.random.choice(toks, size=(num_examples, example_len))
    out_tags = np.empty_like(seqs, dtype=seqs.dtype)
    samples = []
    for i in xrange(num_examples):
        if forward:
            out_tags[i, 0] = prior[seqs[i, 0]]
            for t in xrange(1, example_len):
                out_tags[i, t] = trm[out_tags[i, t - 1], seqs[i, t - 1]]
        else:
            out_tags[i, -1] = prior[seqs[i, -1]]
            for t in xrange(example_len - 1):
                out_tags[i, t] = trm[out_tags[i, t - 1], seqs[i, t + 1]]
        samples.append(Sample(tokens=seqs[i].tolist(), tags=out_tags[i].tolist()))
    return samples


def to_samples(normalizing_samples_extractor, data):
    return normalizing_samples_extractor(
        FuzzyNLUFormat.parse_iter(sum(data, [])).items
    )


@pytest.fixture(scope='module')
def samples_small(normalizing_samples_extractor):
    data = to_samples(normalizing_samples_extractor, weather_data(single_intent=True).values())
    rng.shuffle(data)
    return data


@pytest.fixture(scope='module')
def features_extractor(parser):
    return create_features_extractor(
        parser=parser,
        ngrams={'n': 1},
        lemma={},
        ner={},
    )


@pytest.fixture(scope='module')
def features_extractor_embeddings(dummy_embeddings, parser):
    return create_features_extractor(
        parser=parser,
        ngrams={'n': 1},
        lemma={},
        ner={},
        embeddings={'file': dummy_embeddings}
    )


@pytest.mark.slowtest
def test_gridsearch_crf(samples_small, features_extractor):
    tagger = CRFTokenTagger()
    report = tagger.gridsearch(features_extractor(samples_small), {
        'window_size': [1, 5]
    }, n_jobs=1)
    assert report[report.rank_test_score == 1].param_splicerfeaturespostprocessor__window_size.iloc[0] == 5


@pytest.mark.slowtest
def test_crossvalidation_crf(samples_small, features_extractor):
    assert CRFTokenTagger().crossvalidation(features_extractor(samples_small), average='macro', n_jobs=1) > 0.6


def test_gridsearch_on_validation_data(mocker, nlu_demo_data, features_extractor, normalizing_samples_extractor):
    features = {
        intent: features_extractor(
            to_samples(normalizing_samples_extractor, [data])
        ) for intent, data in nlu_demo_data.iteritems()
    }
    intent_mapper = {
        'goodbye': 'greeting',
        'how.*': 'thanks'
    }
    f1_score_mock = mocker.patch.object(
        vins_core.nlu.token_classifier.TokenClassifierScorer, '_get_score', return_value=0
    )
    clf = create_token_classifier('maxent')
    valid_data = OrderedDict([
        ('goodbye', features_extractor(to_samples(normalizing_samples_extractor, [['пока']]))),
        ('how_are_you', features_extractor(to_samples(normalizing_samples_extractor, [['как дела']])))
    ])
    clf.gridsearch(
        features, {'C': [1e-2, 1e-3]}, intent_mapper=intent_mapper, validation_data=valid_data, average='wmacro'
    )
    f1_score_mock.assert_called_with(['greeting', 'thanks'], ['greeting', 'thanks'], 'wmacro')
