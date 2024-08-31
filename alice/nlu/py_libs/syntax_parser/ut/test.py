# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import yatest.common

import logging

from alice.nlu.py_libs.syntax_parser.parser import Parser

logger = logging.getLogger(__name__)


def test_apply():
    parser = Parser.load(yatest.common.binary_path('alice/nlu/py_libs/syntax_parser/model'))

    expected_parse = {
        'tokens': ['мама', 'мыла', 'раму'],
        'lemmas': ['мама', 'мыть', 'рама'],
        'heads': [2, 0, 2],
        'head_tags': ['nsubj', 'root', 'obj'],
        'grammar_values': [
            'NOUN|Animacy=Anim|Case=Nom|Gender=Fem|Number=Sing',
            'VERB|Aspect=Imp|Gender=Fem|Mood=Ind|Number=Sing|Tense=Past|VerbForm=Fin|Voice=Act',
            'NOUN|Animacy=Inan|Case=Acc|Gender=Fem|Number=Sing'
        ]
    }

    parse = parser.parse('мама мыла раму', predict_syntax=True, return_embeddings=True)
    assert parse['embeddings'].shape == (1, 3, 512)
    del parse['embeddings']
    assert parse == expected_parse

    parse = parser.parse(['мама', 'мыла', 'раму'], predict_syntax=True, return_embeddings=True)
    assert parse['embeddings'].shape == (1, 3, 512)
    del parse['embeddings']
    assert parse == expected_parse
