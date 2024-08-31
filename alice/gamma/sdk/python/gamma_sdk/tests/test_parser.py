# coding: utf-8

import re

import pytest
from gamma_sdk.inner.lexer import lexer, Variable, Token, Text
from gamma_sdk.inner.parser import parse, SequenceNode, TextNode, VariableNode, OrNode, AnyNode, MaybeNode


def TEXT(v, suffix=False, prefix=False, inflect=False):
    return Token('TEXT', Text(value=v, suffix=suffix, prefix=prefix, inflect=inflect))


def VARIABLE(n, t=None, v=None):
    return Token('VARIABLE', Variable(n, t or n, v))


@pytest.mark.parametrize('text, tokens', [
    (
        'Привет тебе прекрасное создание',
        [TEXT('Привет'), TEXT('тебе'), TEXT('прекрасное'), TEXT('создание')]
    ),
    (
        '     ',
        []
    ),
    (
        'Кажется, это $Animal',
        [TEXT('Кажется,'), TEXT('это'), VARIABLE('Animal')]
    ),
    (
        'Это стоит \\$100',
        [TEXT('Это'), TEXT('стоит'), TEXT('$100')]
    ),
    (
        '\\(\\*\\*\\*\\)',
        [TEXT('(***)')]
    ),
    (
        'Кажется, это $First:Animal, а не $Second:Animal.cow',
        [TEXT('Кажется,'), TEXT('это'), VARIABLE('First', t='Animal'),
         TEXT(','), TEXT('а'), TEXT('не'), VARIABLE('Second', t='Animal', v='cow')]
    ),
])
def test_lexer(text, tokens):
    assert list(lexer(text)) == tokens


@pytest.mark.parametrize('text, tree', [
    ('Кажется, это $Animal',
     SequenceNode(sequence=[
         TextNode('Кажется,'),
         TextNode('это'),
         VariableNode(name='Animal', type='Animal'),
     ])),
    ('(п|П)ривет',
     SequenceNode(sequence=[
         OrNode(alternatives=[
             TextNode('п'), TextNode('П')
         ]),
         TextNode('ривет')
     ])),
    ('* *идет* *',
     SequenceNode(sequence=[
         AnyNode(), TextNode('идет', prefix=True, suffix=True), AnyNode()
     ])),
    ('* *ид*ет* *',
     SequenceNode(sequence=[
         AnyNode(), TextNode('ид', prefix=True, suffix=True), TextNode('ет', suffix=True), AnyNode()
     ])),
    ('* [$maybe|$think|давай|как насчет|конечно] [это] $SoundAnimal [$maybe|$think|что ли|давай] *',
     SequenceNode(sequence=[
         AnyNode(),
         MaybeNode(
             expression=OrNode(alternatives=[
                 VariableNode(name='maybe', type='maybe'),
                 VariableNode(name='think', type='think'),
                 TextNode('давай'),
                 SequenceNode(sequence=[TextNode('как'), TextNode('насчет')]),
                 TextNode('конечно'),
             ]),
         ),
         MaybeNode(
             expression=TextNode('это')
         ),
         VariableNode(name='SoundAnimal', type='SoundAnimal'),
         MaybeNode(
             expression=OrNode(alternatives=[
                 VariableNode(name='maybe', type='maybe'),
                 VariableNode(name='think', type='think'),
                 SequenceNode(sequence=[TextNode('что'), TextNode('ли')]),
                 TextNode('давай'),
             ]),
         ),
         AnyNode()
     ])),
])
def test_parser(text, tree):
    assert parse(text).expression == tree


@pytest.mark.parametrize('pattern, regex, examples', [
    ('* *идет* * $Animal',
     r'^.*(?:\b| ).*идет.*(?:\b| ).*(?:\b| )(?P<Animal>[^ ]+?)$',
     ['придет серенький волчок', ' идет коровка']),
    ('[может|давай] (сходим|пойдем) *',
     r'^(?:может|давай)?(?:\b| )(?:сходим|пойдем)(?:\b| ).*$',
     ['сходим куда-нибудь']),
    ('[переведи] из \\$ *',
     r'^(?:переведи)?(?:\b| )из(?:\b| )\$(?:\b| ).*$',
     ['переведи из $ в €']),
    ('[переведи] из (\\$|€) в (€|\\$)',
     r'^(?:переведи)?(?:\b| )из(?:\b| )(?:\$|€)(?:\b| )в(?:\b| )(?:€|\$)$',
     ['переведи из $ в €'])
])
def test_regex(pattern, regex, examples):
    result_regex = parse(pattern).to_regex()
    assert result_regex == regex
    for example in examples:
        assert re.match(result_regex, example, flags=re.UNICODE) is not None
