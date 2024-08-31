# coding: utf-8
import pytest
from gamma_sdk.inner.matcher import SimpleMatcher, EntityExtractor, Entity, Variable


@pytest.fixture
def simple_matcher():
    matcher = SimpleMatcher(
        {
            'hello': '* (привет|здравствуй|здравствуйте) *',
            'goodbye': '* (пока|до свидания) *',
            'cow': '* (точно|точняк|определенно) $Animal.cow *',
            'animal': '* ([это] $Animal:Animal | $Animal2:Animal [это]) *',
        }
    )
    return matcher


@pytest.mark.parametrize('request_, response', [
    ('привет тебе', 'И вам привет'),
    ('сколько стоит айфон?', 'Не знаю что и ответить'),
    ('ну ладно, пока', 'И вам пока'),
])
def test_simple_match(simple_matcher, request_, response):
    hypothesis, variables = next(simple_matcher.match(request_))

    assert {
        None: 'Не знаю что и ответить',
        'hello': 'И вам привет',
        'goodbye': 'И вам пока',
    }[hypothesis] == response


@pytest.fixture
def entity_extractor():
    return EntityExtractor(entities={
        'Animal': {
            'cat': [
                'кошка', 'кошки', 'кошке', 'кошкой', 'кошку',
                'кошечка', 'кошечки', 'кошечке', 'кошечкой', 'кошечку',
                'кот', 'кота', 'коту', 'коте', 'котом',
                'котик', 'котика', 'котику', 'котике', 'котиком',
            ],
            'dog': [
                'собака', 'собаки', 'собаку', 'собаке', 'собакой',
                'собачка', 'собачки', 'собачку', 'собачке', 'собачкой',
                'пес', 'пса', 'псу', 'псе', 'псом',
                'песик', 'песика', 'песику', 'песике', 'песиком',
            ],
            'cow': [
                'корова', 'коровы', 'корове', 'корову', 'коровой',
                'коровка', 'коровки', 'коровке', 'коровку', 'коровкой',
            ],
        },
        'Words': {
            'hello': ['привет', 'здравствуйте'],
            'maybe': ['может быть', 'возможно', 'кажется', 'наверное'],
        }
    })


@pytest.mark.parametrize('example, entities', [
    ('может быть, это котик с кошечкой', {
        'Animal': [
            Entity(
                type='Animal',
                value='cat',
                begin=3,
                end=4
            ),
            Entity(
                type='Animal',
                value='cat',
                begin=5,
                end=6
            ),
        ],
        'Words': [
            Entity(
                type='Words',
                value='maybe',
                begin=0,
                end=2
            ),
        ],
    }),
    ('котик с кошечкой и коровкой, наверное', {
        'Animal': [
            Entity(
                type='Animal',
                value='cat',
                begin=0,
                end=1
            ),
            Entity(
                type='Animal',
                value='cat',
                begin=2,
                end=3
            ),
            Entity(
                type='Animal',
                value='cow',
                begin=4,
                end=5
            ),
        ],
        'Words': [
            Entity(
                type='Words',
                value='maybe',
                begin=5,
                end=6
            ),
        ],
    }),
])
def test_entity_extractor(entity_extractor, example, entities):
    result = entity_extractor.get_entities(example)
    assert result == entities


@pytest.mark.parametrize('text, hypothesis, variables', [
    ('наверное это коровка', 'animal', {'Animal': Variable(type='Animal', value='cow')}),
    ('я знаю что это точно корова', 'cow', {'Animal': Variable(type='Animal', value='cow')}),
    ('это точно корова мычит', 'cow', {'Animal': Variable(type='Animal', value='cow')}),
])
def test_match_with_entities(simple_matcher, entity_extractor, text, hypothesis, variables):
    result_hypothesis, result_variables = next(simple_matcher.match(text, entity_extractor.get_entities(text)))
    assert result_hypothesis == hypothesis
    assert result_variables == variables
