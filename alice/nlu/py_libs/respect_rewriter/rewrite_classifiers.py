# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import codecs
import json
import logging
import numpy as np
import os
import re
import shutil
import six
import tarfile
import tempfile

from string import punctuation

from alice.nlu.py_libs.tokenizer import tokenize
from alice.nlu.py_libs.tf_model_applier import TfModelApplier

logger = logging.getLogger(__name__)


class _BaseClassifier(object):
    def __init__(self, model_dir, embeddings_dir):
        with open(os.path.join(model_dir, 'model_description.json')) as f:
            self._model_config = json.load(f)

        input_nodes = [
            self._model_config['inputs']['token_ids'],
            self._model_config['inputs']['lengths'],
        ]
        output_nodes = [self._model_config['outputs']['class_probs']]

        self._model = TfModelApplier(os.path.join(model_dir, 'model.pb'), input_nodes, output_nodes)

        with open(os.path.join(model_dir, 'trainable_embeddings.json')) as f:
            trainable_embeddings = json.load(f)

        self._embeddings_matrix, self._word_to_index = self._load_embeddings(trainable_embeddings, embeddings_dir)

    @staticmethod
    def _load_embeddings(trainable_embeddings, embeddings_dir):
        special_tokens = list(trainable_embeddings.keys())
        trainable_embeddings = np.array([trainable_embeddings[tok] for tok in special_tokens])
        special_tokens = ['PAD'] + special_tokens

        embeddings_matrix = np.load(os.path.join(embeddings_dir, 'embeddings.npy'))

        embeddings_matrix = np.concatenate((
            np.zeros((1, 300)),
            trainable_embeddings,
            embeddings_matrix
        ), axis=0)

        with codecs.open(os.path.join(embeddings_dir, 'embeddings.dict'), encoding='utf8') as f:
            index_to_word = special_tokens + [line.rstrip() for line in f]
            word_to_index = {word: index for index, word in enumerate(index_to_word)}

        return embeddings_matrix, word_to_index

    def predict_proba(self, sentences):
        lengths = [len(sent) for sent in sentences]
        embeddings = np.zeros((len(sentences), max(lengths), 300))
        for i, sentence in enumerate(sentences):
            for j, token in enumerate(sentence):
                embeddings[i, j] = self._embeddings_matrix[self._word_to_index.get(token, 0)]

        probs = self._model({
            self._model_config['inputs']['token_ids']: embeddings,
            self._model_config['inputs']['lengths']: lengths,
        })[0].squeeze(-1)

        return probs

    @classmethod
    def load_from_archive(cls, model_path, embeddings_path):
        tmp_dir = None
        try:
            tmp_dir = tempfile.mkdtemp()
            with tarfile.open(model_path, "r:") as archive:
                archive.extractall(tmp_dir)

            with tarfile.open(embeddings_path, "r:") as archive:
                archive.extractall(tmp_dir)

            return cls(os.path.join(tmp_dir, 'model'), os.path.join(tmp_dir, 'embeddings'))
        finally:
            if tmp_dir:
                shutil.rmtree(tmp_dir)


class RewritableSentenceClassifier(_BaseClassifier):
    def __init__(self, *args, **kwargs):
        super(RewritableSentenceClassifier, self).__init__(*args, **kwargs)
        self._split_punct = re.compile('([{}])([ {}])'.format(punctuation, punctuation), re.U)

    def _preprocess(self, sentence):
        if isinstance(sentence, list):
            tokens = [six.ensure_text(token.lower()).replace('ё', 'е') for token in sentence]
            return tokens

        sentence = six.ensure_text(sentence).lower()
        sentence = sentence.replace('ё', 'е')
        sentence = self._split_punct.sub(r' \g<1> \g<2> ', sentence)
        tokens = [token.text for token in tokenize(sentence)]
        return tokens

    def predict_proba(self, sentences):
        sentences = [self._preprocess(sentence) for sentence in sentences]
        return super(RewritableSentenceClassifier, self).predict_proba(sentences)


class RewritablePositionsClassifier(_BaseClassifier):
    def predict_proba(self, tokens):
        tokens = [token.replace('ё', 'е').lower() for token in tokens]

        probs = super(RewritablePositionsClassifier, self).predict_proba([tokens])
        return probs.T[0]
