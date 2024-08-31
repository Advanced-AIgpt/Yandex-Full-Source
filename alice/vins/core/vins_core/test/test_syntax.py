# -*- coding: utf-8 -*-
from __future__ import unicode_literals
import pytest

from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.registry import create_sample_processor
from vins_core.nlu.syntax import Token, TokenStorage, SyntaxParser, SyntaxParsingException


def create_sample(utt):
    extractor = SamplesExtractor(pipeline=[create_sample_processor('wizard')], allow_wizard_request=True)
    return extractor([utt])[0]


@pytest.mark.parametrize('sentence,reference_attrs', [
    (
        'раз два три',
        [
            {
                'start': 0,
                'end': 3,
                '_grammem': ['S', 'acc', 'sg', 'm', 'inan'],
                'text': 'раз'
            },
            {
                'start': 4,
                'end': 7,
                '_grammem': ['NUM', 'acc', 'm', 'inan'],
                'text': 'два'
            },
            {
                'start': 8,
                'end': 11,
                '_grammem': ['NUM', 'nom'],
                'text': 'три'
            }
        ]
    ),
    (
        'санкт-петербург',
        [
            {
                'start': 0,
                'end': 5,
                '_grammem': ['COM'],
                'text': 'санкт'
            },
            {
                'start': 6,
                'end': 15,
                '_grammem': ['S', 'geo', 'acc', 'sg', 'm', 'inan'],
                'text': 'петербург'
            }
        ]
    ),
    (
        'Большая буква',
        [
            {
                'start': 0,
                'end': 7,
                '_grammem': ['A', 'nom', 'sg', 'plen', 'f'],
                'text': 'большая'
            },
            {
                'start': 8,
                'end': 13,
                '_grammem': ['S', 'nom', 'sg', 'f', 'inan'],
                'text': 'буква'
            }
        ]
    )

])
def test_extract_tokens(sentence, reference_attrs):
    tokens = Token.extract_tokens(create_sample(sentence))

    assert len(tokens) == len(reference_attrs)
    for token, token_attrs in zip(tokens, reference_attrs):
        for attr, value in token_attrs.iteritems():
            assert getattr(token, attr) == value


def test_iter_tokens():
    token = Token(sentence='привет как дела', text='дела', lemma='дело',
                  grammem='a b c', start=11, end=15, other_grammems=['a1 b1 c1'])
    assert [t._grammem for t in token.iter_tokens()] == [['a', 'b', 'c'], ['a1', 'b1', 'c1']]


@pytest.mark.parametrize('sentence,ref_tokens', [
    ('одинтокен', ['одинтокен']),
    ('раз два три', ['раз', 'два', 'три']),
    ('санкт-петербург', ['санкт', 'петербург'])
])
def test_token_storage(sentence, ref_tokens):
    ts = TokenStorage.from_text(sentence)
    tokens = ts.tokens
    for real, ref in zip(tokens, ref_tokens):
        assert real.text == ref
        assert ref in ts


@pytest.mark.parametrize('sentence,ref_noun_phrases', [
    ('алексей навальный', [
        {'text': 'алексей навальный', 'head': 'алексей'}
    ]),
    ('коса юлии тимошенко', [
        {'text': 'коса', 'head': 'коса'},
        {'text': 'юлии тимошенко', 'head': 'юлии'},
        {'text': 'коса юлии тимошенко', 'head': 'коса'}
    ]),
    ('загадочная история бенджамина баттона фильм 2009', [
        {'text': 'загадочная история', 'head': 'история'},
        {'text': 'бенджамина баттона', 'head': 'бенджамина'},
        {'text': 'загадочная история бенджамина баттона фильм', 'head': 'фильм'}
    ]),
    ('кто такой верка сердючка', [
        {'text': 'верка сердючка', 'head': 'верка'}
    ]),
    ('кто отец брэда питта', [
        {'text': 'отец', 'head': 'отец'},
        {'text': 'брэда питта', 'head': 'брэда'},
        {'text': 'отец брэда питта', 'head': 'отец'}
    ]),
    ('билл клинтон', [
        {'text': 'билл клинтон', 'head': 'билл'}
    ]),
    ('теория большого взрыва', [
        {'text': 'теория', 'head': 'теория'},
        {'text': 'большого взрыва', 'head': 'взрыва'},
        {'text': 'теория большого взрыва', 'head': 'теория'}
    ])
])
def test_syntax_parser(sentence, ref_noun_phrases):
    parser = SyntaxParser()
    parser_result = parser(create_sample(sentence))

    real_noun_phrases = {p.text: p.head.text for p in parser_result.phrases}
    for ref_np in ref_noun_phrases:
        text = ref_np['text']
        head_text = ref_np['head']
        assert text in real_noun_phrases
        assert head_text == real_noun_phrases[text]


@pytest.mark.xfail(reason=SyntaxParsingException)
def test_syntax_parser_fails():
    parser = SyntaxParser()
    utterance = u'\U0001f604нуиненада'
    parser(create_sample(utterance))
