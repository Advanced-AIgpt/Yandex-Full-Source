# coding: utf-8
from __future__ import unicode_literals

import pytest
import numpy as np
from io import BytesIO, TextIOWrapper
from library.python import resource
from alice.nlu.py_libs.utils.fuzzy_nlu_format import FuzzyNLUFormat, FuzzyNLUTemplate


NLU_NUMBER_ERROR_MSG = r'Either a number of template instances or `all` keyword is expected after "\(" in "%s" for utterance "%s"'


@pytest.fixture
def nlu_templates():
    resource_data = resource.find('resfs/file/first_name_ru.txt')
    data = []
    for line in TextIOWrapper(BytesIO(resource_data), encoding='utf-8'):
        line = line.strip()
        if line and not line.startswith('#'):
            data.append(line)
    return {'first_name_ru': FuzzyNLUTemplate(data=data)}


@pytest.mark.parametrize('source_text, expected_slots', [
    ('хочу "веселое"(search_text) видео про "котиков"(search_text)', [False, False]),
    ('хочу "веселое"(search_text) видео про "котиков"(+search_text)', [False, True]),
    ('хочу "веселое"(+search_text) видео про "котиков"(search_text)', [False, False]),
    ('хочу "веселое"(+search_text) видео про "котиков"(+search_text)', [False, True]),
])
def test_continuation_slots_parse(source_text, expected_slots):
    item = FuzzyNLUFormat.parse_one(source_text)
    for resulting_slot, expected_continuation in zip(item.slots, expected_slots):
        assert resulting_slot.is_continuation == expected_continuation


@pytest.mark.parametrize("template, markup", [
    ('позвони "@first_name_ru(1:datv)"(slot)', 'позвони амету'),
    ('позвони "@first_name_ru(1:datv,pl)"(slot)', 'позвони аметам')
])
def test_patterns_infl(nlu_templates, template, markup):
    rng = np.random.RandomState(123)
    nlu_sources = FuzzyNLUFormat.parse_iter(
        [template],
        name='test',
        templates=nlu_templates,
        rng=rng
    )
    assert nlu_sources.items[0].text == markup


def test_patterns_raw(nlu_templates):
    nlu_sources = FuzzyNLUFormat.parse_iter(
        ['позвони "@first_name_ru(100)"(slot)'],
        name='test',
        templates=nlu_templates
    )
    assert len(nlu_sources.items) == 100


def test_patterns_unique(nlu_templates):
    nlu_sources = FuzzyNLUFormat.parse_iter(
        ['позвони "@first_name_ru(100)"(slot)'],
        name='test',
        templates=nlu_templates
    )
    assert len(nlu_sources.items) == len(set(nlu_sources.items))


def test_patterns_escaped_ats(nlu_templates):
    rng = np.random.RandomState(123)
    nlu_sources = FuzzyNLUFormat.parse_iter(
        ['позвони "@@@first_name_ru(1)"(slot)'],
        name='test',
        templates=nlu_templates,
        rng=rng
    )
    assert nlu_sources.items[0].text == 'позвони @амет'


@pytest.mark.parametrize('utterance', [
    'позвони "@first_name_ru"(slot)',
    'позвони "@@@first_name_ru"(slot)'
])
def test_unescaped_patterns_without_brackets(nlu_templates, utterance):
    with pytest.raises(ValueError, match=NLU_NUMBER_ERROR_MSG):
        FuzzyNLUFormat.parse_iter(
            [utterance],
            name='test',
            templates=nlu_templates
        )


@pytest.mark.parametrize('src_utterance, generated_utterance', [
    ('позвони "@@first_name_ru"(slot)', 'позвони @first_name_ru'),
    ('позвони "@@@@first_name_ru"(slot)', 'позвони @@first_name_ru')
])
def test_escaped_patterns_without_brackets(nlu_templates, src_utterance, generated_utterance):
    nlu_sources = FuzzyNLUFormat.parse_iter(
        [src_utterance],
        name='test',
        templates=nlu_templates
    )
    assert len(nlu_sources.items) == 1
    assert nlu_sources.items[0].text == generated_utterance


@pytest.mark.parametrize('utterance', [
    ('позвони "@first_name_ru(all)"(slot)'),
    ('позвони "@first_name_ru(all:gen)"(slot)')
])
def test_expand_to_all(nlu_templates, utterance):
    nlu_sources = FuzzyNLUFormat.parse_iter(
        [utterance],
        name='test',
        templates=nlu_templates
    )
    assert len(nlu_sources.items) > 1
    assert '@' not in nlu_sources.items[0].text


@pytest.mark.parametrize('utterance', [
    ('позвони "@first_name_ru()"(slot)'),
    ('позвони "@first_name_ru(:gen)"(slot)')
])
def test_no_number(nlu_templates, utterance):
    with pytest.raises(ValueError, match=NLU_NUMBER_ERROR_MSG):
        FuzzyNLUFormat.parse_iter(
            [utterance],
            name='test',
            templates=nlu_templates
        )
