# -*- coding: utf-8 -*-

import json
import numpy as np

from six.moves import zip

from alice.nlu.py_libs.syntax_parser.vectorizers import VectorizerStack
from alice.nlu.py_libs.tf_model_applier import TfModelApplier


class UncontextualizedCachableEmbedder(object):
    def __init__(self, vectorizer_config, embedder_path, model_config, output_size):
        self._model = TfModelApplier(path=embedder_path, **model_config)
        self._vectorizer = VectorizerStack.from_config(vectorizer_config)
        self._model_config = model_config
        self._output_size = output_size

        # TODO(dan-anastasev): should be lru-cache or something
        self._embeddings_cache = {}

    def __call__(self, tokens):
        sentence_length = len(tokens)

        token_embeddings = np.zeros((1, sentence_length, self._output_size), dtype=np.float32)

        unknown_tokens, unknown_token_indices = [], []
        for token_index, token in enumerate(tokens):
            token_embedding = self._embeddings_cache.get(token)
            if token_embedding is not None:
                token_embeddings[0, token_index] = token_embedding
            else:
                unknown_tokens.append(token)
                unknown_token_indices.append(token_index)

        if unknown_tokens:
            vectorized_inputs = self._vectorizer(unknown_tokens)
            vectorized_inputs = {
                key: np.expand_dims(val, 0) for key, val in vectorized_inputs.items()
            }

            embedder_inputs = {
                self._model_config['input_names'][0]: vectorized_inputs['elmo']
            }
            embedder_outputs = self._model(embedder_inputs)[0]

            unknown_token_embeddings = np.concatenate((embedder_outputs, vectorized_inputs['morpho']), -1)
            token_embeddings[0, unknown_token_indices] = unknown_token_embeddings

            for unknown_token, unknown_token_embedding in zip(unknown_tokens, unknown_token_embeddings[0]):
                self._embeddings_cache[unknown_token] = unknown_token_embedding

        return token_embeddings


class ParserModel(object):
    def __init__(self, model_config_path, embedder_path, model_path):
        with open(model_config_path) as f:
            self._model_config = json.load(f)

        self._embedder = UncontextualizedCachableEmbedder(
            vectorizer_config=self._model_config['vectorizer'],
            embedder_path=embedder_path,
            model_config=self._model_config['embedder'],
            output_size=self._model_config['embedder_output_size']
        )
        self._model = TfModelApplier(
            path=model_path,
            **self._model_config['model']
        )

    def __call__(self, tokens, predict_grammar_values, predict_lemmas, predict_syntax, return_embeddings):
        model_inputs = {
            self._model_config['model']['input_names'][0]: self._embedder(tokens),
            self._model_config['model']['input_names'][1]: [len(tokens)],
        }

        output_node_indices = []
        if predict_grammar_values:
            output_node_indices.append(0)
        if predict_lemmas:
            output_node_indices.append(1)
        if predict_syntax:
            output_node_indices.append(2)
        if return_embeddings:
            output_node_indices.append(3)

        outputs = self._model(model_inputs, output_node_indices)

        outputs_dict = {}
        for output, node_index in zip(outputs, output_node_indices):
            if node_index == 0:
                outputs_dict['gram_vals'] = output
            elif node_index == 1:
                outputs_dict['lemmas'] = output
            elif node_index == 2:
                outputs_dict['batch_energy'] = output
            elif node_index == 3:
                outputs_dict['embeddings'] = output

        return outputs_dict
