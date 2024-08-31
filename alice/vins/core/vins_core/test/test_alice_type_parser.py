# coding: utf-8
from __future__ import unicode_literals

import pytest

from copy import deepcopy

from vins_core.common.annotations import AnnotationsBag, WizardAnnotation
from vins_core.common.entity import Entity
from vins_core.common.sample import Sample
from vins_core.dm.request import create_request
from vins_core.ner.wizard import NluWizardAliceTypeParserTime
from vins_core.utils.misc import gen_uuid_for_tests


_WIZARD_RESPONSE = {
    'AliceTypeParserTime': {
        'Result': {
            'ParsedEntitiesByType': [{
                'key': 'time',
                'value': {
                    'ParsedEntities': [{
                        'StartToken': 4,
                        'EndToken': 10,
                        'Value': '{"hours": 11, "minutes": 2, "period": "am"}',
                        'Text': '11 часов утра и 2 минуты',
                        'Type': 'time'
                    }]
                }
            }],
            'Tokens': ['алиса', 'будильник', 'на', 'сегодня', '11', 'часов', 'утра', 'и', '2', 'минуты']
        }
    }
}


@pytest.fixture
def parser(parser_factory):
    return parser_factory.create_parser(
        fst_parsers=['time'],
        additional_parsers=[NluWizardAliceTypeParserTime()]
    )


@pytest.mark.parametrize('experiments, expected_entities', [
    (
        None,
        [
            Entity(start=3, end=6, type='TIME', value={'hours': 11, 'period': 'am'}),
            Entity(start=7, end=9, type='TIME', value={'minutes': 2}),
            Entity(start=3, end=9, type='TIME', value={'hours': 11, 'minutes': 2, 'period': 'am'})
        ]
    ),
])
def test_parse_rich_annotation(parser, experiments, expected_entities):
    text = 'будильник на сегодня 11 часов утра и 2 минуты'

    req_info = create_request(gen_uuid_for_tests(), experiments=experiments)

    wizard_annotation = WizardAnnotation(
        markup={},
        rules=_WIZARD_RESPONSE,
        token_alignment=range(len(text.split()))
    )
    annotations = AnnotationsBag({'wizard': wizard_annotation})
    sample = Sample.from_string(text, annotations=annotations)
    entities = parser(sample, req_info=req_info)

    assert len(entities) == len(expected_entities)
    for entity in entities:
        assert entity in expected_entities


def test_parse_empty_annotation(parser):
    text = 'будильник на сегодня 11 часов утра и 2 минуты'

    wizard_time_empty_responses = []

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    wizard_response['AliceTypeParserTime']['Result']['ParsedEntitiesByType'][0]['value']['ParsedEntities'].pop()
    wizard_time_empty_responses.append(wizard_response)

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    del wizard_response['AliceTypeParserTime']['Result']['ParsedEntitiesByType'][0]['value']['ParsedEntities']
    wizard_time_empty_responses.append(wizard_response)

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    wizard_response['AliceTypeParserTime']['Result']['ParsedEntitiesByType'][0]['key'] = 'datetime'
    wizard_time_empty_responses.append(wizard_response)

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    del wizard_response['AliceTypeParserTime']['Result']['ParsedEntitiesByType']
    wizard_time_empty_responses.append(wizard_response)

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    del wizard_response['AliceTypeParserTime']['Result']
    wizard_time_empty_responses.append(wizard_response)

    wizard_response = deepcopy(_WIZARD_RESPONSE)
    del wizard_response['AliceTypeParserTime']
    wizard_time_empty_responses.append(wizard_response)

    req_info = create_request(gen_uuid_for_tests())

    for wizard_response in wizard_time_empty_responses:
        wizard_annotation = WizardAnnotation(
            markup={},
            rules=wizard_response,
            token_alignment=range(len(text.split()))
        )
        annotations = AnnotationsBag({'wizard': wizard_annotation})
        sample = Sample.from_string(text, annotations=annotations)
        entities = parser(sample, req_info=req_info)

        assert len(entities) == 2
        for entity in entities:
            assert entity.value in [{'hours': 11, 'period': 'am'}, {'minutes': 2}]


@pytest.mark.parametrize('text, wizard_response, alignment', [
    (
        'посчитай 25 * 5',
        {
            'AliceTypeParserTime': {
                'Result': {
                    'ParsedEntitiesByType': [
                        {
                            'key': 'time',
                            'value': {
                                'ParsedEntities': [
                                    {
                                        'StartToken': 1,
                                        'EndToken': 2,
                                        'Value': '{\"minutes\": 25, \"minutes_relative\": true}',
                                        'Text': '25',
                                        'Type': 'time'
                                    },
                                    {
                                        'StartToken': 3,
                                        'EndToken': 4,
                                        'Value': '{\"hours\": 5, \"hours_relative\": true}',
                                        'Text': '5',
                                        'Type': 'time'
                                    }
                                ]
                            }
                        }
                    ],
                    'Tokens': ['алиса', 'посчитай', '25', '*', '5']
                }
            }
        },
        [-1, -1, 0, 1, 3]
    ),
    (
        'жургенова 28 / 1',
        {
            'AliceTypeParserTime': {
                'Result': {
                    'ParsedEntitiesByType': [
                        {
                            'key': 'time',
                            'value': {
                                'ParsedEntities': [
                                    {
                                        'StartToken': 3,
                                        'EndToken': 4,
                                        'Value': '{\"hours\":1}',
                                        'Text': '1',
                                        'Type': 'time'
                                    },
                                    {
                                        'StartToken': 1,
                                        'EndToken': 2,
                                        'Value': '{\"minutes\": 28, \"minutes_relative\": true}',
                                        'Text': '28',
                                        'Type': 'time'
                                    },
                                    {
                                        'StartToken': 3,
                                        'EndToken': 4,
                                        'Value': '{\"hours\": 1, \"hours_relative\": true}',
                                        'Text': '1',
                                        'Type': 'time'
                                    }
                                ]
                            }
                        }
                    ],
                    'Tokens': ['жургенова', '28', '/', '1']
                }
            }
        },
        [0, 1, 3]
    )
])
def test_parsing_with_wrong_wizard_alignment(text, wizard_response, alignment, parser):
    wizard_annotation = WizardAnnotation(
        markup={},
        rules=wizard_response,
        token_alignment=alignment
    )
    annotations = AnnotationsBag({'wizard': wizard_annotation})
    sample = Sample.from_string(text, annotations=annotations)
    entities = parser(sample)
    assert entities
