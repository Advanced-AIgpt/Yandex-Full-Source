# -*- coding: utf-8 -*-

import pytest
import mock
import numpy as np

from collections import OrderedDict
from uuid import uuid4
from itertools import izip

from vins_core.nlu.classifier import Classifier
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.nlu.flow_nlu import FlowNLU
from vins_core.ner.fst_presets import FstParserFactory
from vins_core.nlu.features_extractor import create_features_extractor
from vins_core.nlu.reranker.factor_calcer import FactorCalcer, Factor, FactorCalcerContext
from vins_core.nlu.reranker.factors import (
    WordNNFactor, IntentBasedFactor, BagOfEntitiesFactor, DenseFeatureFactor, KNNFactor
)
from vins_core.nlu.reranker.reranker import (
    DescendingByScoresReranker, DescendingByScoresAndTransitionModelReranker,
    CombinedReranker, ExperimentBasedReranker, CatboostReranker, create_default_reranker
)
from vins_core.dm.form_filler.form_candidate import FormCandidate
from vins_core.dm.form_filler.models import Form
from vins_core.dm.form_filler.dialog_manager import DialogManager
from vins_core.dm.intent import Intent
from vins_core.nlu.features.extractor.base import SparseFeatureValue
from vins_core.dm.request import create_request
from vins_core.utils.misc import gen_uuid_for_tests
from vins_core.dm.session import Session
from personal_assistant.app import FormSetup
from personal_assistant.bass_result import BassFormSetupMeta

DSSM_RESOURCE = 'resource://dssm_embeddings/tf_model'


@pytest.fixture(scope='module')
def features_extractor(dummy_embeddings):
    return create_features_extractor(
        parser=None,
        dssm_embeddings={'resource': DSSM_RESOURCE},
        embeddings={'file': dummy_embeddings}
    )


@pytest.fixture(scope='module')
def sample_features(samples_extractor, features_extractor):
    return features_extractor(samples_extractor([u'поставь будильник на 6 утра']))[0]


@pytest.fixture(scope='module')
def form_candidates():
    return [
        FormCandidate(Form('form_0'), IntentCandidate('intent_0', score=1., is_in_fixlist=True), 0),
        FormCandidate(Form('form_1'), IntentCandidate('intent_1', score=0.3, is_active_slot=True), 1),
        FormCandidate(Form('form_2'), IntentCandidate('intent_2', score=0.95, has_priority_boost=False), 2),
        FormCandidate(Form('form_3'), IntentCandidate('intent_3', score=0.95, transition_model_score=1.00001,
                                                      has_priority_boost=True), 3),
        FormCandidate(Form('form_4'), IntentCandidate('intent_4', score=0.9, transition_model_score=1.1), 4),
    ]


@pytest.fixture(scope='module')
def default_params():
    return dict(
        feature_extractors=[
            {'type': 'ngrams', 'id': 'word', 'n': 1},
        ],
        intent_classifiers=[{
            'name': 'intent_classifier_0',
            'model': 'maxent',
            'l2reg': 1e-2,
            'features': ['word', 'bigram', 'lemma', 'ner', 'postag']
        }],
        utterance_tagger={
            'model': 'crf',
            'features': ['word', 'lemma', 'ner'],
            'params': {'intent_conditioned': True}
        }
    )


class FakeApp(object):
    def setup_forms(self, forms, **kwargs):
        return [
            FormSetup(
                meta=BassFormSetupMeta(is_feasible=True),
                form=form,
                precomputed_data={'index': index + 1}
            )
            for index, form in enumerate(forms)
        ]


class DummyFactor(Factor):
    def append_factor_values(self, context, factor_values):
        factor_values.append(np.tile(context.sample_features.dense['dssm_embeddings'], (len(context.classes), 1)))

    def get_factor_value_names(self):
        return []


class DummyRerankerModel(object):
    def predict(self, data):
        return data[:, 0]


class InvertingScoreRerankerModel(DummyRerankerModel):
    def predict(self, data):
        return -data[:, 0]


class DummyFactorCalcer():
    def __call__(self, sample_features, form_candidate, required_factors):
        return np.array([[candidate.intent.score] for candidate in form_candidate])


class InvertingScoreFactorCalcer():
    def __call__(self, sample_features, form_candidate, required_factors):
        return np.array([[-candidate.intent.score] for candidate in form_candidate])


def catboost_reranker(model=None, factor_calcer=None):
    model = model if model is not None else DummyRerankerModel()
    factor_calcer = factor_calcer if factor_calcer is not None else FactorCalcer({'scenarios': KNNFactor()})

    return CatboostReranker(
        model=model,
        factor_calcer=factor_calcer,
        used_factors=['scenarios'],
        known_intents=['intent_0', 'intent_1', 'intent_2', 'intent_3', 'intent_4']
    )


@pytest.mark.parametrize('required_factors', (['scenarios'], ['scenarios', 'dummy']))
def test_factor_calcer(sample_features, required_factors, form_candidates):
    factor_calcer = FactorCalcer({'scenarios': KNNFactor(), 'dummy': DummyFactor()})

    computed_factors = factor_calcer(sample_features, form_candidates, required_factors)

    assert np.all(computed_factors[:, 0] == np.array([candidate.intent.score for candidate in form_candidates]))

    if len(required_factors) == 1:
        assert computed_factors.shape[1] == 1
    else:
        for i in xrange(len(form_candidates)):
            assert np.all(computed_factors[i, 1:] == sample_features.dense['dssm_embeddings'])


def _check_rerankers_called_correctly(call_expected_reranker, call_not_expected_rerankers, *args):
    call_expected_reranker.assert_called_once_with(*args)
    for reranker in call_not_expected_rerankers:
        reranker.assert_not_called()


@pytest.mark.parametrize('experiment', [None, 'first_experiment', 'second_experiment'])
def test_experiment_based_reranker(mocker, sample_features, form_candidates, experiment):
    default_reranker_stub = mocker.stub('default_reranker')
    first_experiment_reranker_stub = mocker.stub('first_experiment_reranker')
    second_experiment_reranker_stub = mocker.stub('second_experiment_reranker')

    experiment_to_reranker = OrderedDict()
    experiment_to_reranker['first_experiment'] = first_experiment_reranker_stub
    experiment_to_reranker['second_experiment'] = second_experiment_reranker_stub

    reranker = ExperimentBasedReranker(experiment_to_reranker=experiment_to_reranker,
                                       default_reranker=default_reranker_stub)

    req_info = create_request(gen_uuid_for_tests(), experiments=[experiment])

    reranker(sample_features, form_candidates, req_info)

    if experiment is None:
        _check_rerankers_called_correctly(
            default_reranker_stub,
            [first_experiment_reranker_stub, second_experiment_reranker_stub],
            sample_features, form_candidates, req_info
        )
    elif experiment == 'first_experiment':
        _check_rerankers_called_correctly(
            first_experiment_reranker_stub,
            [default_reranker_stub, second_experiment_reranker_stub],
            sample_features, form_candidates, req_info
        )
    elif experiment == 'second_experiment':
        _check_rerankers_called_correctly(
            second_experiment_reranker_stub,
            [first_experiment_reranker_stub, default_reranker_stub],
            sample_features, form_candidates, req_info
        )


def test_sorting_by_scores_reranker(sample_features, form_candidates):
    reranker = DescendingByScoresReranker()
    result_candidates = reranker(sample_features, form_candidates)

    expected_candidates_order = ['intent_0', 'intent_2', 'intent_3', 'intent_4', 'intent_1']
    assert [candidate.intent.name for candidate in result_candidates] == expected_candidates_order


def test_sorting_by_scores_and_transition_model_reranker(sample_features, form_candidates):
    reranker = DescendingByScoresAndTransitionModelReranker()
    result_candidates = reranker(sample_features, form_candidates)

    expected_candidates_order = ['intent_4', 'intent_0', 'intent_3', 'intent_2', 'intent_1']
    assert [candidate.intent.name for candidate in result_candidates] == expected_candidates_order


@pytest.mark.parametrize('reranker, reorders',
                         ([catboost_reranker(), False],
                          [catboost_reranker(model=InvertingScoreRerankerModel()), True]))
def test_catboost_reranker_different_models(reranker, reorders, sample_features, form_candidates):
    def sort_key(candidate):
        return -candidate.intent.score if reorders else candidate.intent.score

    result_candidates = reranker(sample_features, form_candidates)
    result_candidate_names = [candidate.intent.name for candidate in result_candidates]

    initial_candidate_names = [candidate.intent.name for candidate
                               in sorted(form_candidates, key=sort_key, reverse=True)]
    assert initial_candidate_names == result_candidate_names


@pytest.mark.parametrize('reranker, reorders',
                         ([catboost_reranker(factor_calcer=DummyFactorCalcer()), False],
                          [catboost_reranker(factor_calcer=InvertingScoreFactorCalcer()), True]))
def test_catboost_reranker_different_factor_calcers(reranker, reorders, sample_features, form_candidates):
    def sort_key(candidate):
        return -candidate.intent.score if reorders else candidate.intent.score

    result_candidates = reranker(sample_features, form_candidates)
    result_candidate_names = [candidate.intent.name for candidate in result_candidates]

    initial_candidate_names = [candidate.intent.name for candidate
                               in sorted(form_candidates, key=sort_key, reverse=True)]
    assert initial_candidate_names == result_candidate_names


def test_catboost_reranker_not_applied_to_unknown_intents(mocker, sample_features):
    reranker = catboost_reranker(factor_calcer=DummyFactorCalcer())

    form_candidates = [
        FormCandidate(Form('form_0'), IntentCandidate('intent_0', score=0.3), 0),
        FormCandidate(Form('form_1'), IntentCandidate('intent_1', score=0.2), 1),
        FormCandidate(Form('unknown_intent'), IntentCandidate('unknown_intent', score=1.), 2),
        FormCandidate(Form('form_2'), IntentCandidate('intent_2', score=0.7), 3),
    ]

    result_candidates = reranker(sample_features, form_candidates)

    expected_result_candidates = [
        FormCandidate(Form('form_2'), IntentCandidate('intent_2', score=0.7), 3),
        FormCandidate(Form('form_0'), IntentCandidate('intent_0', score=0.3), 0),
        FormCandidate(Form('unknown_intent'), IntentCandidate('unknown_intent', score=1.), 2),
        FormCandidate(Form('form_1'), IntentCandidate('intent_1', score=0.2), 1),
    ]

    assert len(result_candidates) == len(expected_result_candidates)
    for result_candidate, expected_candidate in izip(result_candidates, expected_result_candidates):
        assert result_candidate.intent.name == expected_candidate.intent.name
        assert result_candidate.intent.score == pytest.approx(expected_candidate.intent.score)


@pytest.mark.parametrize('forms_count', [0, 1])
def test_catboost_reranker_not_applied_to_trivial_list(mocker, sample_features, forms_count):
    factor_calcer_stub = mocker.stub('factor_cacler_stub')
    reranker = catboost_reranker(factor_calcer=factor_calcer_stub)

    form_candidates = [FormCandidate(Form('some_form'), IntentCandidate(name='some_intent'), i)
                       for i in xrange(forms_count)]
    result_candidates = reranker(sample_features, form_candidates)

    assert result_candidates == form_candidates
    factor_calcer_stub.assert_not_called()


@pytest.mark.parametrize('active_slot_reranker',
                         ([DescendingByScoresReranker(), catboost_reranker()]))
@pytest.mark.parametrize('fixlist_reranker',
                         ([DescendingByScoresReranker(), catboost_reranker()]))
@pytest.mark.parametrize('passed_candidates_reranker',
                         ([DescendingByScoresReranker(), catboost_reranker()]))
@pytest.mark.parametrize('apply_to_first_non_empty_group_only', [True, False])
def test_combined_reranker(sample_features, form_candidates, active_slot_reranker, fixlist_reranker,
                           passed_candidates_reranker, apply_to_first_non_empty_group_only):
    reranker = CombinedReranker(
        active_slot_reranker, fixlist_reranker, passed_candidates_reranker, apply_to_first_non_empty_group_only
    )

    result_candidates = reranker(sample_features, form_candidates)

    assert result_candidates[0].intent.name == 'intent_1', 'Active slot is sorted higher'

    if not apply_to_first_non_empty_group_only:
        assert result_candidates[1].intent.name == 'intent_0', 'Intents from train are sorted higher'

        expected_passed_candidates_order = sorted(
            form_candidates[2:], key=lambda candidate: candidate.intent.score, reverse=True
        )
        expected_passed_candidate_names = [candidate.intent.name for candidate in expected_passed_candidates_order]
        result_passed_candidate_names = [candidate.intent.name for candidate in result_candidates[2:]]
        assert result_passed_candidate_names == expected_passed_candidate_names


def test_nlu_reranker_application(default_params, form_candidates, mocker):
    nlu = FlowNLU(
        samples_extractor={},
        fst_parser_factory=FstParserFactory(None, {}),
        fst_parsers=[],
        **default_params
    )
    nlu.load()
    nlu.add_classifier(classifier=Classifier())
    intents_data = [
        ('intent_0', 0.5, False),
        ('intent_1', 0.5, False),
        ('intent_2', 0.5, False),
        ('intent_3', 0.5, False),
        ('intent_4', 0.5, False),
        ('fallback_intent', 0.1, True),
    ]
    for intent_name, fallback_threshold, fallback in intents_data:
        nlu.add_intent(
            intent_name=intent_name,
            fallback_threshold=fallback_threshold,
            fallback=fallback
        )

    nlu._predict_intents = mock.MagicMock(return_value=[x.intent for x in form_candidates])
    nlu._reranker = create_default_reranker()

    dm = DialogManager(
        [Intent(candidate.intent.name) for candidate in form_candidates],
        {candidate.intent.name: candidate.form for candidate in form_candidates},
        nlu=nlu,
        nlg=None,
        max_intents=10
    )
    mocker.patch.object(dm, "_ask_slot")
    mocker.patch.object(dm, "_submit_form")

    session = Session(app_id='123', uuid=uuid4())
    nlu_result = dm.handle(create_request(uuid=session.uuid, utterance='Привет'),
                           session=session, app=FakeApp(), response=None)

    reranked_intents = [res['intent_name'] for res in nlu_result.semantic_frames]

    assert reranked_intents[0] == 'intent_1', 'Active slot is sorted higher'
    assert reranked_intents[1] == 'intent_0', 'Intents from train are sorted higher'

    expected_passed_candidates_order = sorted(
        form_candidates[2:], key=lambda candidate: candidate.intent.score, reverse=True
    )
    expected_passed_candidate_names = [candidate.intent.name for candidate in expected_passed_candidates_order]
    result_passed_candidate_names = [candidate for candidate in reranked_intents[2:]]
    assert result_passed_candidate_names == expected_passed_candidate_names


@pytest.mark.parametrize('batch_first', [True, False])
def test_word_nn_factor_inputs(sample_features, batch_first):
    embeddings_name = 'embeddings'
    model = WordNNFactor(model_name='test_model', model_applier=None, class_to_indices=None, model_output_size=4,
                         embeddings_name=embeddings_name, batch_first=batch_first)

    feed = model._get_inputs(sample_features)

    seq_dim, batch_dim = (1, 0) if batch_first else (0, 1)
    assert feed['dense_seq'].shape[seq_dim] == len(sample_features) and feed['dense_seq'].shape[batch_dim] == 1
    assert feed['dense_seq'].shape[2] == sample_features.dense_seq[embeddings_name].shape[1]


@pytest.mark.parametrize('batch_first', [True, False])
@pytest.mark.parametrize('return_distribution', [True, False])
def test_word_nn_factor_outputs(mocker, sample_features, return_distribution, batch_first):
    model_output_size = 4

    factor = WordNNFactor(
        model_name='test_model', model_applier=None,
        class_to_indices={'intent_1': [0, 1], 'intent_2': [2], 'intent_3': [3]},
        model_output_size=model_output_size, return_distribution=return_distribution,
        embeddings_name='embeddings', batch_first=batch_first
    )

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_3'])

    model_predictions = np.array([0.3, 0.15, 0.2, 0.35])

    factor._model = mocker.MagicMock()
    factor._model.encode.return_value = np.array([model_predictions])

    factor_values = []
    factor.append_factor_values(context, factor_values)

    assert len(factor_values) == 1

    prediction_vector_dim = model_output_size if return_distribution else 1
    assert factor_values[0].shape == (2, prediction_vector_dim)

    if return_distribution:
        for i in xrange(len(context.classes)):
            assert np.all(factor_values[0][i] == model_predictions)
    else:
        assert factor_values[0][0] == model_predictions[0] + model_predictions[1]
        assert factor_values[0][1] == model_predictions[3]


@pytest.mark.parametrize('return_distribution', [True, False])
def test_word_nn_factor_outputs_missing_embeddings(mocker, sample_features, return_distribution):
    model_output_size = 4

    factor = WordNNFactor(
        model_name='test_model', model_applier=None,
        class_to_indices={'intent_1': [0, 1], 'intent_2': [2], 'intent_3': [3]},
        model_output_size=model_output_size, return_distribution=return_distribution,
        embeddings_name='missing_embeddings'
    )

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_3'])

    model_predictions = np.array([0.3, 0.15, 0.2, 0.35])

    factor._model = mocker.MagicMock()
    factor._model.encode.return_value = np.array([model_predictions])

    factor_values = []
    factor.append_factor_values(context, factor_values)

    assert len(factor_values) == 1

    prediction_vector_dim = model_output_size if return_distribution else 1
    assert np.all(factor_values[0] == np.zeros((2, prediction_vector_dim)))


def test_intent_based_factor():
    intent_index = {
        'intent_1': 0,
        'intent_2': 1,
        'intent_3': 2
    }
    intent_features = np.array([
        [1, 2, 0],
        [2, 1, 0],
        [3, 0, 1],
        [0, 0, 0]
    ])
    factor = IntentBasedFactor(intent_index, intent_features)

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_3'],
                                  original_classes=['intent_1', 'intent_3'])

    factor_values = []
    factor.append_factor_values(context, factor_values)

    assert len(factor_values) == 1
    assert factor_values[0].shape == (2, 3)
    assert np.all(factor_values[0] == intent_features[[0, 2]] * 2)


@pytest.mark.parametrize('has_artist', [True, False])
def test_bag_of_entities_factor_applied_to_tags_seq(sample_features, has_artist):
    entity_type_to_index = {
        'ALBUM': 0,
        'ARTIST': 1
    }
    factor = BagOfEntitiesFactor(entity_type_to_index, 'ner', apply_to_tag_sequence=True)

    sample_features.sparse_seq['ner'] = [
        [SparseFeatureValue('B-ALBUM'), SparseFeatureValue('B-GEO')],
        [SparseFeatureValue('I-ALBUM'), SparseFeatureValue('I-GEO')]
    ]
    if has_artist:
        sample_features.sparse_seq['ner'][0].append(SparseFeatureValue('B-ARTIST'))

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_2'])

    factor_values = []
    factor.append_factor_values(context, factor_values)

    assert len(factor_values) == 1
    assert factor_values[0].shape == (2, 2)

    expected = np.array([[1, 0], [1, 0]])
    if has_artist:
        expected[:, 1] = 1
    assert np.all(factor_values[0] == expected)


@pytest.mark.parametrize('has_artist', [True, False])
def test_bag_of_entities_factor_applied_to_entities(sample_features, has_artist):
    entity_type_to_index = {
        'ALBUM': 0,
        'ARTIST': 1
    }
    factor = BagOfEntitiesFactor(entity_type_to_index, 'ner', apply_to_tag_sequence=False)

    sample_features.sparse['ner'] = [SparseFeatureValue('ALBUM'), SparseFeatureValue('GEO')]
    if has_artist:
        sample_features.sparse['ner'].append(SparseFeatureValue('ARTIST'))

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_2'])

    factor_values = []
    factor.append_factor_values(context, factor_values)

    assert len(factor_values) == 1
    assert factor_values[0].shape == (2, 2)

    expected = np.array([[1, 0], [1, 0]])
    if has_artist:
        expected[:, 1] = 1
    assert np.all(factor_values[0] == expected)


def test_dense_factor(sample_features):
    feature_name = 'some_feature'
    feature_size = 10
    feature_list = ['feature_{}'.format(i) for i in xrange(feature_size)]
    dense_value = np.arange(feature_size)
    factor = DenseFeatureFactor(feature_name, feature_list)

    context = FactorCalcerContext(sample_features, classes=['intent_1', 'intent_2'])
    factor_values = []

    factor.append_factor_values(context, factor_values)

    sample_features.dense[feature_name] = dense_value
    factor.append_factor_values(context, factor_values)

    assert np.all(factor_values[0] == np.zeros(feature_size))
    assert np.all(factor_values[1] == dense_value)
