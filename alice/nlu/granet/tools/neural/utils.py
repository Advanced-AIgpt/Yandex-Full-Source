# coding: utf-8

from __future__ import unicode_literals

import codecs
import numpy as np
import os
import re


_TOKENS_PATTERN = re.compile(ur"\b(\w+)\b|'([^']+)'\(([a-z_]+)\)", re.U)
_MARKUP_PATTERN = re.compile(ur"'([^']+)'\(([a-z_]+)\)", re.U)


class TokenEmbedder(object):
    def __init__(self, embeddings, word_to_index):
        self._embeddings = embeddings
        self._word_to_index = word_to_index

    def __call__(self, tokens):
        return np.stack([self._embeddings[self._word_to_index.get(token, 0)] for token in tokens], axis=0)


def parse_nlu_item(text):
    text = text.lower()
    tokens, tags = [], []
    for match in _TOKENS_PATTERN.finditer(text):
        untagged_token, tagged_text, tag = match.groups()
        if untagged_token is not None:
            tokens.append(untagged_token)
            tags.append('O')
        else:
            tagged_tokens = tagged_text.split()
            tokens.extend(tagged_tokens)

            cur_tags = ['I-' + tag] * len(tagged_tokens)
            cur_tags[0] = 'B-' + cur_tags[0][2:]
            tags.extend(cur_tags)

    assert len(tokens) == len(tags)
    return tokens, tags


def _extract_slots(tokens, tags):
    prev_slot_name, slot_tokens = None, []

    assert len(tokens) == len(tags)
    for token, tag in zip(tokens, tags):
        slot_name = 'O' if tag == 'O' else tag[2:]
        if slot_name != prev_slot_name:
            if prev_slot_name and slot_tokens:
                yield (slot_tokens, prev_slot_name)
            prev_slot_name, slot_tokens = slot_name, []
        slot_tokens.append(token)
    if prev_slot_name and slot_tokens:
        yield (slot_tokens, prev_slot_name)


def to_nlu_line(tokens, tags):
    nlu_line = []
    for tokens, slot in _extract_slots(tokens, tags):
        if slot != 'O':
            nlu_line.append("'{}'({})".format(' '.join(tokens), slot))
        else:
            nlu_line.append(' '.join(tokens))
    return ' '.join(nlu_line)


def get_text_without_markup(text):
    def tagged_replacement(match):
        return match.group(1)

    return _MARKUP_PATTERN.sub(tagged_replacement, text)


def iterate_lines(path):
    with codecs.open(path, encoding='utf8') as f:
        for line in f:
            line = line.strip()
            if not line or '\t' not in line:
                continue

            weight, text = line.split('\t')
            if weight.lower() == 'weight' or text.lower() == 'text':
                continue

            yield weight, text


def load_embeddings(embeddings_dir):
    embeddings_matrix = np.load(os.path.join(embeddings_dir, 'embeddings.npy'))
    embeddings_matrix = np.concatenate((np.zeros((1, 300)), embeddings_matrix), axis=0)

    with codecs.open(os.path.join(embeddings_dir, 'embeddings.dict'), encoding='utf8') as f:
        index_to_word = ['[PAD]'] + [line.rstrip() for line in f]
        word_to_index = {word: index for index, word in enumerate(index_to_word)}

    return embeddings_matrix, index_to_word, word_to_index
