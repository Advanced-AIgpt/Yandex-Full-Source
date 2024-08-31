# coding: utf-8
from __future__ import unicode_literals

import pytest

from vins_core.ner.fst_normalizer import NluFstNormalizer, NORMALIZER_CONFIG, DEFAULT_RU_NORMALIZER_NAME


NORMALIZER_RESOURCE = NORMALIZER_CONFIG[DEFAULT_RU_NORMALIZER_NAME]['normalizer']['resource']


@pytest.fixture(scope='module')
def f_with_all_fst():
    norm = NluFstNormalizer._load_normalizer_from_resource(NORMALIZER_RESOURCE)

    def tmp(utt):
        return norm.normalize(utt).decode('utf-8')

    return tmp


@pytest.fixture(scope='module')
def f_without_any_fst():
    norm = NluFstNormalizer._load_normalizer_from_resource(
        NORMALIZER_RESOURCE,
        fst_black_list=[
            'number.convert_cardinal',
            'simple_conversions.plus_minus',
            'simple_conversions.replace_unconditionally',
        ])

    def tmp(utt):
        return norm.normalize(
            utt.encode('utf-8')
        ).decode('utf-8')

    return tmp


@pytest.mark.parametrize("utterance, expected", [
    ('2 + 2', 'два плюс два'),
])
def test_pynorm_with_all_fst(f_with_all_fst, utterance, expected):
    assert f_with_all_fst(utterance) == expected


@pytest.mark.parametrize("utterance,expected", [
    ('2', '2'),
    ('два', 'два'),
    ('третий сезон', 'третий сезон'),
    ('сука', 'сука'),
    ('2 + 2', '2 + 2'),
    ('2 + (2 * 3) / 5', '2 + ( 2 * 3 ) / 5'),
])
def test_pynorm_without_any_fst(f_without_any_fst, utterance, expected):
    assert f_without_any_fst(utterance) == expected
