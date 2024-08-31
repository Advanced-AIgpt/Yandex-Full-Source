# coding: utf-8
from __future__ import unicode_literals

import pytest
import pandas as pd

from ..process_nlu_on_dataset import _classify, _report, _crossval


@pytest.fixture(scope='module')
def validation_set_tsv(tmpdir_factory):
    tmpdir = tmpdir_factory.mktemp('test_data')
    out_file = str(tmpdir.join('test_validation_set'))
    pd.DataFrame([
        ('intent1', 'текст'),
        ('intent1', 'фраза'),
        ('intent2', 'фраза'),
        ('micro1', 'nlu')
    ], columns=['intent', 'text']).to_csv(out_file, sep=b'\t', encoding='utf-8', index=False)
    yield out_file
    tmpdir.remove()


@pytest.mark.slowtest
@pytest.mark.parametrize('recall_by_slots', (False, True))
def test_classify(tmpdir_factory, test_app, validation_set_tsv, recall_by_slots):
    tmpdir = tmpdir_factory.mktemp('test_results')
    output_file = str(tmpdir.join('test_results'))
    _classify(
        app_name=test_app,
        input_file=validation_set_tsv,
        format='tsv',
        output_file=output_file,
        nbest=100,
        force_intent=None,
        text_col='text',
        intent_col='intent'
    )
    _report(results_file=output_file, recall_by_slots=recall_by_slots)


@pytest.mark.slowtest
@pytest.mark.parametrize('recall_by_slots', (False, True))
def test_crossval_taggers(tmpdir, test_app, recall_by_slots):
    output_file = tmpdir.join('test_results').strpath
    _crossval(
        app_name=test_app,
        output_file=output_file,
        num_runs=3,
        classifiers=(),
        taggers=True,
        feature_cache=None,
        validation=0.3,
        validate_intent=None
    )
    _report(results_file=output_file + '.taggers.pkl', recall_by_slots=recall_by_slots)


@pytest.mark.slowtest
@pytest.mark.parametrize('classifiers', (('test_clf', 'test_fallback_clf'), ))
def test_crossval_classifiers(tmpdir, test_app, classifiers):
    output_file = tmpdir.join('test_results').strpath
    _crossval(
        app_name=test_app,
        output_file=output_file,
        num_runs=3,
        classifiers=classifiers,
        taggers=True,
        feature_cache=None,
        validation=0.3,
        validate_intent=None
    )
    _report(results_file=output_file + '.classifiers.pkl')
