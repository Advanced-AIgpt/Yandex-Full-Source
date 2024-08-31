# -*- coding: utf-8 -*-

import logging
import pytest
import re

from alice.nlu.py_libs.tokenizer import tokenize, sentenize

logger = logging.getLogger(__name__)


def test_tokenize():
    text = 'мама мыла раму'
    tokens = list(tokenize(text))

    merged_text = text[:tokens[0].start]
    for i, token in enumerate(tokens):
        if i > 0:
            merged_text += text[tokens[i - 1].stop: tokens[i].start]
        assert text[token.start: token.stop] == token.text
        merged_text += text[token.start: token.stop]
    merged_text += text[tokens[-1].stop:]

    assert merged_text == text

    tokens = [token.text for token in tokens]
    assert tokens == text.split()


def test_tokenize_with_splits():
    text = 'А сам-то чего не спишь?'
    tokens = list(tokenize(text, splits=[(re.compile('(\w)-то'), '\g<1> - то')]))

    merged_text = text[:tokens[0].start]
    for i, token in enumerate(tokens):
        if i > 0:
            merged_text += text[tokens[i - 1].stop: tokens[i].start]
        assert text[token.start: token.stop] == token.text
        merged_text += text[token.start: token.stop]
    merged_text += text[tokens[-1].stop:]

    assert merged_text == text

    tokens = [token.text for token in tokens]
    assert tokens == ['А', 'сам', '-', 'то', 'чего', 'не', 'спишь', '?']


def test_sentenize():
    text = 'А.С. Пушкин родился в 1799 году. Умер в 1837.'
    sentences = list(sentenize(text))
    logger.info(sentences)
    for sentence in sentences:
        assert text[sentence.start: sentence.stop] == sentence.text
    sentences = [sentence.text for sentence in sentences]
    assert sentences == ['А.С. Пушкин родился в 1799 году.', 'Умер в 1837.']
