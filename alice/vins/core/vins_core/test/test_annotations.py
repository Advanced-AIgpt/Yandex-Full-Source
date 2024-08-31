# coding: utf-8
from __future__ import unicode_literals

from vins_core.common.annotations import (
    AnnotationsBag,
    BaseAnnotation,
    register_annotation,
    get_annotation_id,
    get_annotation_class,
)
from vins_core.common.annotations.entitysearch import EntitySearchAnnotation, EntityFeatures, Entity
from vins_core.nlu.anaphora.mention import Mention
from vins_core.nlu.syntax import Token, NounPhrase

import attr
import pytest

EXAMPLE_ANNOTATION_ID = '__example_annotation'


@attr.s
class ExampleAnnotation(BaseAnnotation):
    first_field = attr.ib()
    second_field = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(first_field=data.get('first_field'), second_field=data.get('second_field'))

    def trim(self):
        self.first_field = None


register_annotation(ExampleAnnotation, EXAMPLE_ANNOTATION_ID)


def test_util_functions():
    example_annotation = ExampleAnnotation(first_field='a value', second_field='another value')

    assert get_annotation_id(ExampleAnnotation) == EXAMPLE_ANNOTATION_ID
    assert get_annotation_class(EXAMPLE_ANNOTATION_ID) == ExampleAnnotation

    serialized = {
        'first_field': 'a value',
        'second_field': 'another value'
    }
    assert example_annotation.to_dict() == serialized
    assert ExampleAnnotation.from_dict(serialized) == example_annotation

    # Test trimming.
    trimmed_serialized = {
        'first_field': None,
        'second_field': 'another value'
    }
    example_annotation.trim()
    assert example_annotation.to_dict() == trimmed_serialized
    assert ExampleAnnotation.from_dict(trimmed_serialized) == example_annotation


def test_annotations_bag():
    bag = AnnotationsBag()
    bag['key1'] = ExampleAnnotation(first_field='field11', second_field='field12')
    bag.add('key2', ExampleAnnotation(first_field='field21', second_field='field22'))

    serialized = {
        'key1': {
            'type': EXAMPLE_ANNOTATION_ID,
            'value': {
                'first_field': 'field11',
                'second_field': 'field12'
            }
        },
        'key2': {
            'type': EXAMPLE_ANNOTATION_ID,
            'value': {
                'first_field': 'field21',
                'second_field': 'field22'
            }
        }
    }

    assert bag.to_dict() == serialized
    assert AnnotationsBag.from_dict(serialized)._storage == bag._storage

    # Test trimming.
    trimmed_serialized = {
        'key1': {
            'type': EXAMPLE_ANNOTATION_ID,
            'value': {
                'first_field': None,
                'second_field': 'field12'
            }
        },
        'key2': {
            'type': EXAMPLE_ANNOTATION_ID,
            'value': {
                'first_field': None,
                'second_field': 'field22'
            }
        }
    }

    bag.trim()
    assert bag.to_dict() == trimmed_serialized
    assert AnnotationsBag.from_dict(trimmed_serialized)._storage == bag._storage


def test_entity_search_annotation_serialization():
    annotation = EntitySearchAnnotation(
        entities=[Entity('entity1', 'type'), Entity('entity2', 'type')],
        entity_features=EntityFeatures(tags=('tag1', 'tag2'))
    )

    restored_annotation = EntitySearchAnnotation.from_dict(annotation.to_dict())
    assert annotation.entities == restored_annotation.entities
    assert annotation.entity_features == restored_annotation.entity_features

    annotation.trim()
    restored_annotation = EntitySearchAnnotation.from_dict(annotation.to_dict())
    assert annotation.entities == restored_annotation.entities
    assert restored_annotation.entity_features == EntityFeatures()


@pytest.mark.parametrize('strict', [
    pytest.param(True, marks=pytest.mark.xfail(raises=ValueError)),
    False
])
def test_not_registered_annotation(strict):
    bag = AnnotationsBag.from_dict({
        'name': {
            'type': 'aksdjlj3oij20dj20d9j9023__not_registered_annotation_type',
            'value': {
                'some key': 'some value',
                'another key': 'another value'
            }
        }
    }, strict=strict)

    assert len(bag.keys()) == 0


def test_token_serialization():
    token_dict = {'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                  'lemma': 'a', 'grammem': 'S persn acc sg m anim', 'other_grammems': ['S persn gen sg m anim']}
    deserialized_token = Token.from_dict(token_dict)
    constructed_token = Token(sentence='a', start=13, end=18, text='a', lemma='a', grammem='S persn acc sg m anim',
                              other_grammems=['S persn gen sg m anim'])
    assert deserialized_token == constructed_token
    assert constructed_token.to_dict() == token_dict
    assert Token.from_dict(constructed_token.to_dict()) == constructed_token


def test_noun_phrase_serialization():
    noun_phrase_dict = {'tokens': [{'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                                    'lemma': 'a', 'grammem': 'S persn acc sg m anim',
                                    'other_grammems': ['S persn gen sg m anim']},
                                   {'end': 25, 'start': 19, 'text': 'a', 'sentence': 'a',
                                    'lemma': 'a', 'grammem': 'S gen sg m inan', 'other_grammems': None}],
                        'text': 'a',
                        'head': {'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                                 'lemma': 'a', 'grammem': 'S persn acc sg m anim',
                                 'other_grammems': ['S persn gen sg m anim']},
                        'sentence': 'a'}
    tokens = [Token(sentence='a', start=13, end=18, text='a', lemma='a', grammem='S persn acc sg m anim',
                    other_grammems=['S persn gen sg m anim']),
              Token(sentence='a', start=19, end=25, text='a', lemma='a', grammem='S gen sg m inan',
                    other_grammems=None)]
    head = tokens[0]
    constructed_noun_phrase = NounPhrase(sentence='a', tokens=tokens, head=head, text='a')

    assert NounPhrase.from_dict(noun_phrase_dict) == constructed_noun_phrase
    assert noun_phrase_dict == constructed_noun_phrase.to_dict()
    assert constructed_noun_phrase == NounPhrase.from_dict(constructed_noun_phrase.to_dict())


def test_noun_phrase_mention_serialization():
    mention_dict = {'source': 'syntax', 'type': 'np', 'value':
                    {'tokens': [{'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                                 'lemma': 'a', 'grammem': 'S persn acc sg m anim',
                                 'other_grammems': ['S persn gen sg m anim']},
                                {'end': 25, 'start': 19, 'text': 'a', 'sentence': 'a',
                                 'lemma': 'a', 'grammem': 'S gen sg m inan', 'other_grammems': None}],
                     'text': 'a',
                     'head': {'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                              'lemma': 'a', 'grammem': 'S persn acc sg m anim',
                              'other_grammems': ['S persn gen sg m anim']},
                     'sentence': 'a'}}

    tokens = [Token(sentence='a', start=13, end=18, text='a', lemma='a', grammem='S persn acc sg m anim',
                    other_grammems=['S persn gen sg m anim']),
              Token(sentence='a', start=19, end=25, text='a', lemma='a', grammem='S gen sg m inan',
                    other_grammems=None)]
    head = tokens[0]
    constructed_noun_phrase = NounPhrase(sentence='a', tokens=tokens, head=head, text='a')
    constructed_mention = Mention(m_value=constructed_noun_phrase)
    assert Mention.from_dict(mention_dict) == constructed_mention
    assert mention_dict == constructed_mention.to_dict()
    assert constructed_mention == Mention.from_dict(constructed_mention.to_dict())


def test_token_mention_serialization():
    mention_dict = {'source': 'syntax', 'type': 'noun', 'value':
                    {'end': 18, 'start': 13, 'text': 'a', 'sentence': 'a',
                     'lemma': 'a', 'grammem': 'S persn acc sg m anim',
                     'other_grammems': ['S persn gen sg m anim']}}

    token = Token(sentence='a', start=13, end=18, text='a', lemma='a', grammem='S persn acc sg m anim',
                  other_grammems=['S persn gen sg m anim'])
    constructed_mention = Mention(m_value=token)
    assert Mention.from_dict(mention_dict) == constructed_mention
    assert mention_dict == constructed_mention.to_dict()
    assert constructed_mention == Mention.from_dict(constructed_mention.to_dict())
