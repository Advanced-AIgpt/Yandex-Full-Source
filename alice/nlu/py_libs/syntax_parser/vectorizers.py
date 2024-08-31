# -*- coding: utf-8 -*-

import numpy as np
import pymorphy2

from bert.tokenization.full_tokenizer import FullTokenizer


_MAX_WORD_LEN = 50


class BertVectorizer(object):
    NAME = 'bert'

    _CLS_TOKEN_ID = 101
    _SEP_TOKEN_ID = 102

    def __init__(self, tokenizer_path, **kwargs):
        self._tokenizer = FullTokenizer(tokenizer_path, do_lower_case=False)

    def __call__(self, tokens):
        offsets, token_ids = [], []
        token_ids.append(self._CLS_TOKEN_ID)
        for token in tokens:
            subtoken_ids = self._tokenizer.convert_tokens_to_ids(self._tokenizer.tokenize(token))
            offsets.append((len(token_ids), len(token_ids) + len(subtoken_ids)))
            token_ids.extend(subtoken_ids)

        token_ids.append(self._SEP_TOKEN_ID)

        token_ids = np.array(token_ids, dtype=np.int64)
        offsets = np.array(offsets, dtype=np.int64)

        return token_ids, offsets


class ELMoVectorizer(object):
    NAME = 'elmo'

    def __init__(self, **kwargs):
        self._beginning_of_word_character = 258  # <begin word>
        self._end_of_word_character = 259  # <end word>
        self._padding_character = 260  # <padding>

    def __call__(self, tokens):
        char_ids = np.full((len(tokens), _MAX_WORD_LEN), self._padding_character, dtype=np.int64)
        char_ids[:, 0] = self._beginning_of_word_character
        for token_index, token in enumerate(tokens):
            token = token.encode('utf-8', 'ignore')
            token = [
                symbol if isinstance(symbol, int) else ord(symbol)
                for symbol in token[: _MAX_WORD_LEN - 2]
            ]

            char_ids[token_index, 1: len(token) + 1] = token
            char_ids[token_index, len(token) + 1] = self._end_of_word_character

        # +1 one for masking
        char_ids = char_ids + 1

        return char_ids


class MorphoVectorizer(object):
    NAME = 'morpho'

    def __init__(self, **kwargs):
        self._morph = pymorphy2.MorphAnalyzer()
        self._grammeme_to_index = self._build_grammeme_to_index()
        self._morpho_vector_dim = max(self._grammeme_to_index.values()) + 1

    @property
    def morpho_vector_dim(self):
        return self._morpho_vector_dim

    def _build_grammeme_to_index(self):
        grammar_categories = [
            self._morph.TagClass.PARTS_OF_SPEECH,
            self._morph.TagClass.ANIMACY,
            self._morph.TagClass.ASPECTS,
            self._morph.TagClass.CASES,
            self._morph.TagClass.GENDERS,
            self._morph.TagClass.INVOLVEMENT,
            self._morph.TagClass.MOODS,
            self._morph.TagClass.NUMBERS,
            self._morph.TagClass.PERSONS,
            self._morph.TagClass.TENSES,
            self._morph.TagClass.TRANSITIVITY,
            self._morph.TagClass.VOICES
        ]

        grammeme_to_index = {}
        shift = 0
        for category in grammar_categories:
            # TODO: Save grammeme_to_index
            for grammeme_index, grammeme in enumerate(sorted(category)):
                grammeme_to_index[grammeme] = grammeme_index + shift
            shift += len(category) + 1  # +1 to address lack of the category in a parse

        return grammeme_to_index

    def vectorize_word(self, word):
        grammar_vector = np.zeros(self._morpho_vector_dim, dtype=np.float32)
        sum_parses_score = 0.
        for parse in self._morph.parse(word):
            sum_parses_score += parse.score
            for grammeme in parse.tag.grammemes:
                grammeme_index = self._grammeme_to_index.get(grammeme)
                if grammeme_index:
                    grammar_vector[grammeme_index] += parse.score

        if sum_parses_score != 0.:
            grammar_vector /= sum_parses_score

        return grammar_vector

    def __call__(self, tokens):
        matrix = np.stack([self.vectorize_word(token) for token in tokens], axis=0)

        return matrix


class VectorizerStack(object):
    _REGISTERED_VECTORIZERS = [MorphoVectorizer, ELMoVectorizer, BertVectorizer]

    def __init__(self, vectorizers):
        self._vectorizers = vectorizers

    def __call__(self, tokens):
        return {
            vectorizer_key: vectorizer(tokens)
            for vectorizer_key, vectorizer in self._vectorizers.items()
        }

    @classmethod
    def from_config(cls, configs):
        vectorizers = {}
        for config in configs:
            for vectorizer_cls in cls._REGISTERED_VECTORIZERS:
                if config['name'] == vectorizer_cls.NAME:
                    params = config.get('params', {})
                    vector_key = config.get('vector_key', config['name'])
                    vectorizers[vector_key] = vectorizer_cls(**params)

        assert len(vectorizers) == len(configs)

        return cls(vectorizers)
