# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import pytest
import copy

from vins_core.ner.fst_base import Entity
from vins_core.common.sample import Sample
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.common.slots_map_utils import matched, group_by_slot, tags_to_slots


@pytest.mark.parametrize('frame_slot_without_entities,entities,target_types,matching_type,result', [
    (
        {'start': 0, 'end': 1, 'substr': 'Москва'},
        [Entity(start=0, end=1, value='Москва', type='GEO')],
        ['geo'],
        'exact',
        1
    ),
    (
        {'start': 0, 'end': 2, 'substr': 'город Москва'},
        [Entity(start=0, end=1, value='Москва', type='GEO')],
        ['geo'],
        'exact',
        0
    ),
    (
        {'start': 0, 'end': 2, 'substr': 'город Москва'},
        [Entity(start=0, end=1, value='Москва', type='GEO')],
        ['geo'],
        'inside',
        1
    ),
    (
        {'start': 0, 'end': 1, 'substr': 'Москва'},
        [Entity(start=0, end=2, value='Москва великая', type='GEO')],
        ['geo'],
        'inside',
        0
    ),
    (
        {'start': 0, 'end': 2, 'substr': 'город Москва'},
        [Entity(start=1, end=3, value=None, substr='Москва великая', type='GEO')],
        ['geo'],
        'overlap',
        1
    ),
    (
        {'start': 0, 'end': 2, 'substr': 'город Москва'},
        [Entity(start=2, end=3, value=None, substr='Питер', type='GEO')],
        ['geo'],
        'overlap',
        0
    ),
])
def test_matched(frame_slot_without_entities, entities, target_types, matching_type, result):
    frame_slot = copy.deepcopy(frame_slot_without_entities)
    frame_slot['entities'] = copy.deepcopy(entities)

    assert len(list(matched(frame_slot, target_types, matching_type))) == result


def test_overlap_matching_order():
    frame_slot = {'start': 2, 'end': 5, 'substr': 'звук камина и грозы'}
    entities = [
        Entity(start=2, end=5, type=u'IRRELEVANT', value=u'something', substr='', weight=None),
        Entity(start=2, end=3, type=u'AMBIENT_SOUND', value=u'playlist/103372440:1920', substr='', weight=None),
        Entity(start=2, end=5, type=u'AMBIENT_SOUND', value=u'playlist/103372440:1921', substr='', weight=None),
        Entity(start=4, end=5, type=u'AMBIENT_SOUND', value=u'playlist/103372440:1952', substr='', weight=None),
    ]
    frame_slot['entities'] = entities
    matched_entity = next(matched(frame_slot, ['ambient_sound'], 'overlap'), None)
    assert matched_entity == entities[2]


@pytest.mark.parametrize('tokens, tags, result', [
    (
        'хочу большую картошку чизбургер и колу'.split(),
        ['0', 'B-what', 'I-what', 'B-what', '0', 'B-what'],
        [
            (['хочу'], [0], ['0'], '0'),
            (['большую', 'картошку'], [1, 2], ['B-what', 'I-what'], 'what'),
            (['чизбургер'], [3], ['B-what'], 'what'),
            (['и'], [4], ['0'], '0'),
            (['колу'], [5], ['B-what'], 'what')
        ]
    ),
    (
        'большую картошку с собой по деревенски'.split(),
        ['B-what', 'I-what', 'B-how', 'I-how', 'I-what', 'I-what'],
        [
            (['большую', 'картошку'], [0, 1], ['B-what', 'I-what'], 'what'),
            (['с', 'собой'], [2, 3], ['B-how', 'I-how'], 'how'),
            (['по', 'деревенски'], [4, 5], ['I-what', 'I-what'], 'what')
        ]
    ),
])
def test_group_by_slot(tokens, tags, result):
    assert list(group_by_slot(tokens, tags)) == result


@pytest.mark.parametrize("utterance, result", [
    (
        "хочу купить 'iphone 7'(product) и 'samsung'(product) еще 'galaxy'(+product) на 'беру точка ру'(shop)",
        {
            u'product': [
                {'start': 2,
                 'end': 4,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'iphone 7'},
                {'start': 5,
                 'end': 6,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'samsung'},
                {'start': 7,
                 'end': 8,
                 'entities': [],
                 'is_continuation': True,
                 'substr': 'galaxy'}
            ],
            u'shop': [{
                'start': 9,
                'end': 12,
                'entities': [],
                'is_continuation': False,
                'substr': 'беру точка ру'
            }]
        }
    ),
    (
        "возьми 'биг мак'(product) 'стандартную картошку'(product) 'сырный соус'(product) и 'фанту'(product)",
        {
            u'product': [
                {'start': 1,
                 'end': 3,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'биг мак'},
                {'start': 3,
                 'end': 5,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'стандартную картошку'},
                {'start': 5,
                 'end': 7,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'сырный соус'},
                {'start': 8,
                 'end': 9,
                 'entities': [],
                 'is_continuation': False,
                 'substr': 'фанту'}
            ]
        }
    ),
])
def test_tags_to_slots_multiple(utterance, result):
    item = FuzzyNLUFormat.parse_one(utterance)
    sample = Sample.from_nlu_source_item(item)
    slots, free = tags_to_slots(sample.tokens, sample.tags)
    assert slots == result
