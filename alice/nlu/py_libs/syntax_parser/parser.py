# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import numpy as np
import os
import pymorphy2
import shutil
import six
import tarfile
import tempfile

from six.moves import zip

from alice.nlu.py_libs.syntax_parser import pymorphy_tag_converter
from alice.nlu.py_libs.syntax_parser.chu_liu_edmonds import decode_mst
from alice.nlu.py_libs.syntax_parser.lemmatize_helper import LemmatizeHelper
from alice.nlu.py_libs.syntax_parser.model import ParserModel
from alice.nlu.py_libs.syntax_parser.vocabulary import Vocabulary
from text_processing import Tokenizer


def _run_mst_decoding(batch_energy, lengths):
    heads = []
    head_tags = []
    for energy, length in zip(batch_energy, lengths):
        tag_ids = np.argmax(energy, axis=0)
        scores = np.max(energy, axis=0)
        # Although we need to include the root node so that the MST includes it,
        # we do not want any word to be the parent of the root node.
        # Here, we enforce this by setting the scores for all word -> ROOT edges
        # edges to be 0.
        scores[0, :] = 0
        # Decode the heads. Because we modify the scores to prevent
        # adding in word -> ROOT edges, we need to find the labels ourselves.
        instance_heads, _ = decode_mst(scores, length, has_labels=False)

        # Find the labels which correspond to the edges in the max spanning tree.
        instance_head_tags = []
        for child, parent in enumerate(instance_heads):
            instance_head_tags.append(tag_ids[parent, child].item())
        # We don't care what the head or tag is for the root token, but by default it's
        # not necessarily the same in the batched vs unbatched case, which is annoying.
        # Here we'll just set them to zero.
        instance_heads[0] = 0
        instance_head_tags[0] = 0
        heads.append(instance_heads)
        head_tags.append(instance_head_tags)

    return np.stack(heads), np.stack(head_tags)


class Parser(object):
    def __init__(self, vocab_path, lemmatize_helper_path, model_config_path, embedder_path, model_path):
        self._vocab = Vocabulary.from_files(vocab_path)

        self._tokenizer = Tokenizer(
            separator_type='BySense',
            token_types=['Word', 'Number', 'Punctuation', 'SentenceBreak', 'Unknown'],
            languages=['ru', 'en']
        )
        self._lemmatize_helper = LemmatizeHelper.load(lemmatize_helper_path)
        self._model = ParserModel(model_config_path, embedder_path, model_path)
        self._morph = pymorphy2.MorphAnalyzer()

    @property
    def morph(self):
        return self._morph

    def parse(self, sentence, predict_grammar_values=True, predict_lemmas=True,
              predict_syntax=False, return_embeddings=False):
        tokens = self._tokenize(sentence)
        tokens = list(map(six.ensure_text, tokens))

        outputs = self._model(tokens, predict_grammar_values, predict_lemmas, predict_syntax, return_embeddings)
        if predict_syntax:
            outputs['heads'], outputs['head_tags'] = _run_mst_decoding(outputs['batch_energy'], [len(tokens) + 1])

        return self._decode(tokens, outputs, predict_grammar_values, predict_lemmas, predict_syntax, return_embeddings)

    def _tokenize(self, sentence):
        if isinstance(sentence, list):
            return sentence

        if isinstance(sentence, six.string_types):
            return self._tokenizer.tokenize(sentence)

        assert False

    def _decode(self, tokens, model_outputs, predict_grammar_values, predict_lemmas, predict_syntax, return_embeddings):
        outputs = {}

        outputs['tokens'] = tokens

        if predict_grammar_values:
            outputs['grammar_values'] = [
                self._vocab.get_token_from_index(grammar_value_index, 'grammar_value_tags')
                for grammar_value_index in model_outputs['gram_vals'][0]
            ]

        if predict_lemmas:
            outputs['lemmas'] = [
                self._lemmatize_helper.lemmatize(token, lemmatize_rule_index)
                for token, lemmatize_rule_index in zip(tokens, model_outputs['lemmas'][0])
            ]

        if predict_syntax:
            outputs['head_tags'] = [
                self._vocab.get_token_from_index(grammar_value_index, 'head_tags')
                for grammar_value_index in model_outputs['head_tags'][0, 1:]
            ]
            outputs['heads'] = list(map(int, model_outputs['heads'][0, 1:]))

        if return_embeddings:
            outputs['embeddings'] = model_outputs['embeddings']
        return outputs

    def get_pymorphy_form(self, token, expected_grammar_value, force_feats=None, force_pos=True):
        morpho_parses = self._morph.parse(token)
        if len(morpho_parses) == 1:
            return morpho_parses[0]

        force_feats = force_feats or []

        expected_pos, expected_feats = expected_grammar_value.split('|', 1)
        if expected_pos == 'AUX':
            expected_pos = 'VERB'

        expected_feats = set(expected_feats.split('|'))
        expected_forced_feats = sorted([feat for feat in expected_feats if feat.split('=')[0] in force_feats])

        filtered_morpho_parses = []
        for morpho_parse in morpho_parses:
            converted_morpho_parse = pymorphy_tag_converter.to_ud20(str(morpho_parse.tag), token)
            pos, feats = converted_morpho_parse.split()
            if pos != expected_pos and force_pos:
                continue

            feats = set(feats.split('|'))
            forced_feats = sorted([feat for feat in feats if feat.split('=')[0] in force_feats])
            if forced_feats != expected_forced_feats:
                continue

            filtered_morpho_parses.append((morpho_parse, len(expected_feats & feats)))

        if not filtered_morpho_parses:
            return

        filtered_morpho_parses = sorted(filtered_morpho_parses, key=lambda x: x[1], reverse=True)
        return filtered_morpho_parses[0][0]

    @classmethod
    def load(cls, dir_path):
        return cls(
            vocab_path=os.path.join(dir_path, 'vocab'),
            lemmatize_helper_path=os.path.join(dir_path, 'vocab/lemmatizer_info.json'),
            model_config_path=os.path.join(dir_path, 'parser_config.json'),
            embedder_path=os.path.join(dir_path, 'embedder.pb'),
            model_path=os.path.join(dir_path, 'model.pb')
        )

    @classmethod
    def load_from_archive(cls, archive_path):
        tmp_dir = None
        try:
            tmp_dir = tempfile.mkdtemp()
            with tarfile.open(archive_path, "r:") as archive:
                archive.extractall(tmp_dir)

            return cls.load(tmp_dir)
        finally:
            if tmp_dir:
                shutil.rmtree(tmp_dir)
