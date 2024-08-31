# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os
import tempfile
import shutil
import tensorflow as tf
import numpy as np

from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN
from vins_models_tf import TfEncoder, save_encoder_description
from vins_models_tf import save_serialized_model_as_memmapped
from vins_core.nlu.neural.metric_learning.config import MetricLearningConfig  # noqa: UnusedImport
from typing import Dict, AnyStr  # noqa: UnusedImport


class RecurrentStackSequentialEncoder(object):

    _INF = 1e9
    _ENCODER_SCOPE = 'context/context_encoder/stack_seq'
    _COMBINER_SCOPE = 'context/stack'
    _MODEL_FILE_NAME = 'model.mmap'

    def __init__(self,
                 config,  # type: MetricLearningConfig
                 create_for_train,  # type: bool
                 dense_seq_embeddings,  # type: np.memmap
                 input_sizes_map  # type: Dict[AnyStr, int]
                 ):

        """
        :param config: metric learning config
        :param create_for_train: whether to create encoder in train mode or in inference mode
        :param dense_seq_embeddings: word embeddings matrix or None
        :param input_sizes_map: mapping with input sizes for all feature types to construct graph
        """

        self.config = config
        self._create_for_train = create_for_train
        self._dense_seq_embeddings = dense_seq_embeddings
        self._input_sizes_map = input_sizes_map

        self._init_placeholders()
        self._init_embeddings()
        self.output = self._infer()

    def _init_placeholders(self):
        self.placeholders = {
            'lengths': tf.placeholder(dtype=tf.int32, shape=[None], name='lengths'),
            'sparse_seq_ids': tf.sparse_placeholder(dtype=tf.int32, shape=[None, None], name='sparse_seq_ids'),
            'sparse_ids': tf.sparse_placeholder(dtype=tf.int32, shape=[None, None], name='sparse_ids'),
            'dense': tf.placeholder(dtype=tf.float32, shape=[None, self._input_sizes_map['dense']], name='dense'),
            'dense_seq_ids': tf.sparse_placeholder(dtype=tf.int32, shape=[None, None], name='dense_seq_ids'),
            'dense_seq': tf.placeholder(dtype=tf.float32, shape=[None, None, self._input_sizes_map['dense_seq']],
                                        name='dense_seq'),
            'dropouts': tf.placeholder_with_default(
                shape=[len(self.config.encoder_output_dense_layers)],
                input=np.zeros(shape=[len(self.config.encoder_output_dense_layers)], dtype=np.float32)
            )
        }

        if self._dense_seq_embeddings is not None:
            self.placeholders['dense_seq_ids_apply'] = tf.placeholder(
                dtype=tf.float32, shape=[None, None, self._dense_seq_embeddings.shape[-1]], name='dense_seq_ids_apply')

    def _init_embeddings(self):
        if self._create_for_train and self._dense_seq_embeddings is not None:
            params = {
                'trainable': self.config.finetune_embeddings
            }
            if not self.config.finetune_embeddings:
                params['collections'] = [tf.GraphKeys.LOCAL_VARIABLES]

            self._dense_seq_embeddings_variable = tf.Variable(self._dense_seq_embeddings,
                                                              name='dense_seq_embeddings',
                                                              **params)

    def _get_sparse_input(self, input_size, output_size, handle, shape, embeddings=None):
        if input_size:
            ids = self.placeholders['{}_ids'.format(handle)]

            embeddings = embeddings or tf.get_variable(
                name='{}_embeddings'.format(handle), dtype=tf.float32,
                initializer=tf.random_normal(shape=[input_size, output_size], stddev=0.05))

            outputs = tf.reshape(tf.nn.embedding_lookup_sparse(embeddings, ids, sp_weights=None, combiner='mean'),
                                 shape)

            return outputs

    def _get_dense_input(self, input_size, handle):
        if input_size:
            return self.placeholders[handle]

    def _get_dense_seq_ids_input(self):
        if self._dense_seq_embeddings is not None:
            if self._create_for_train:
                ids = self.placeholders['dense_seq_ids']
                ids = tf.sparse_to_dense(ids.indices, tf.shape(ids, out_type=tf.int64), ids.values)

                output = tf.gather_nd(self._dense_seq_embeddings_variable, tf.expand_dims(ids, axis=-1))
                output.set_shape([None, None, self._dense_seq_embeddings.shape[-1]])

                return output
            else:
                return self.placeholders['dense_seq_ids_apply']

    def _tile(self, inputs, maxlen):
        if inputs is not None:
            return tf.tile(tf.expand_dims(inputs, 1), (1, maxlen, 1))

    def _get_stacked_inputs(self):
        batch_size = tf.shape(self.placeholders['lengths'])[0]
        maxlen = tf.reduce_max(self.placeholders['lengths'])

        inputs = [
            self._get_dense_input(self._input_sizes_map['dense_seq'],
                                  'dense_seq'),
            self._get_dense_seq_ids_input(),
            self._get_sparse_input(self._input_sizes_map['sparse_seq'],
                                   self.config.sparse_seq_output_size,
                                   'sparse_seq',
                                   (batch_size, maxlen, self.config.sparse_seq_output_size)),
            self._tile(self._get_dense_input(self._input_sizes_map['dense'],
                                             'dense'),
                       maxlen),
            self._tile(self._get_sparse_input(self._input_sizes_map['sparse'],
                                              self.config.sparse_output_size,
                                              'sparse',
                                              (batch_size, self.config.sparse_output_size)),
                       maxlen),
        ]

        return tf.concat([item for item in inputs if item is not None], axis=-1)

    def _rnn_scope(self, index, direction):
        return 'rnn{}/{}/lstm_cell'.format('_{}'.format(index) if index > 0 else '', direction)

    def _infer(self):
        assert self.config.encoder_num_layers > 0

        with tf.variable_scope(name_or_scope=self._ENCODER_SCOPE):
            seq_inputs = self._get_stacked_inputs()
            seq_inputs = tf.transpose(seq_inputs, [1, 0, 2])

            for i in range(self.config.encoder_num_layers):
                with tf.variable_scope(self._rnn_scope(i, 'fw')) as forward_scope:
                    cell = LSTMBlockFusedCell(num_units=self.config.encoder_num_units)
                    hidden_states_fw, (_, final_hidden_state_fw) = cell(
                        seq_inputs, sequence_length=self.placeholders['lengths'],
                        dtype=tf.float32, scope=forward_scope)

                with tf.variable_scope(self._rnn_scope(i, 'bw')) as backward_scope:
                    cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self.config.encoder_num_units))
                    hidden_states_bw, (_, final_hidden_state_bw) = cell(
                        seq_inputs, sequence_length=self.placeholders['lengths'],
                        dtype=tf.float32, scope=backward_scope)

                seq_inputs = tf.add_n([hidden_states_fw, hidden_states_bw])

            if self.config.encoder_pooling == 'max':
                output = tf.concat([hidden_states_fw, hidden_states_bw], axis=-1)
                invalid_times = 1 - tf.cast(tf.sequence_mask(self.placeholders['lengths'], maxlen=tf.shape(output)[1]),
                                            dtype=tf.float32)
                output = tf.reduce_max(output - self._INF * tf.expand_dims(invalid_times, -1), axis=1)
            elif self.config.encoder_pooling == 'last':
                output = tf.concat([final_hidden_state_fw, final_hidden_state_bw], axis=-1)
            else:
                raise ValueError('Unsupported encoder pooling type: {}'.format(self.config.encoder_pooling))

        for i, layer in enumerate(self.config.encoder_output_dense_layers):
            with tf.variable_scope('{}/dense_{}'.format(self._COMBINER_SCOPE, i - 1) if i > 0 else
                                   '{}/dense'.format(self._ENCODER_SCOPE)) as dense_scope:
                output = tf.contrib.layers.fully_connected(
                    inputs=output,
                    num_outputs=layer['num_units'],
                    activation_fn=tf.nn.relu if layer['relu'] else None,
                    weights_initializer=tf.contrib.layers.xavier_initializer(),
                    biases_initializer=tf.zeros_initializer(),
                    scope=dense_scope)

                output = tf.nn.dropout(x=output, keep_prob=1 - self.placeholders['dropouts'][i])

        if self.config.l2norm:
            output = tf.nn.l2_normalize(x=output, axis=1)

        return output

    def get_input_feed(self, input, is_training):
        result = {
            self.placeholders[key]: value for key, value in input.iteritems()
        }

        dropouts = [layer['dropout'] for layer in self.config.encoder_output_dense_layers]
        result[self.placeholders['dropouts']] = dropouts if is_training else np.zeros_like(dropouts)

        return result

    def encode(self, sess, input):
        input_feed = self.get_input_feed(input, is_training=False)

        return sess.run(self.output, input_feed)

    def _save_frozen_graph(self, sess, model_dir_name, min_conversion_size_bytes):
        graph_def = sess.graph.as_graph_def()
        graph_def = tf.graph_util.convert_variables_to_constants(
            sess,
            graph_def,
            [self.output.op.name]
        )
        model_path = os.path.join(model_dir_name, self._MODEL_FILE_NAME)
        save_serialized_model_as_memmapped(
            model_path,
            graph_def.SerializeToString(),
            min_conversion_size_bytes
        )

    def _save_model_description(self, model_dir_name):
        inputs = {}
        inputs['lengths'] = [self.placeholders['lengths'].name]
        if self._dense_seq_embeddings is not None:
            inputs['dense_seq_ids_apply'] = [self.placeholders['dense_seq_ids_apply'].name]
        for feature, size in self._input_sizes_map.iteritems():
            if not size:
                continue
            placeholder_name = '{}_ids'.format(feature) if feature.startswith('sparse') else feature
            placeholder = self.placeholders[placeholder_name]
            if isinstance(placeholder, tf.SparseTensor):
                inputs[placeholder_name] = [placeholder.indices.name, placeholder.values.name]
            else:
                inputs[placeholder_name] = [placeholder.name]
        model_description = (inputs, self.output.op.name)
        save_encoder_description(model_dir_name, model_description)

    def save_to_dir(self, sess, model_dir_name, min_conversion_size_bytes=100):
        self._save_frozen_graph(sess, model_dir_name, min_conversion_size_bytes)
        self._save_model_description(model_dir_name)

    def convert_to_applier(self, sess):
        tmpdir = None
        try:
            tmpdir = tempfile.mkdtemp()
            self.save_to_dir(sess, tmpdir)
            return RecurrentStackSequentialEncoderApplier(tmpdir)
        finally:
            if tmpdir:
                shutil.rmtree(tmpdir)


class RecurrentStackSequentialEncoderApplier(object):
    def __init__(self, dir_path):
        self.load(dir_path)

    def save(self, dir_path):
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)
        self._model.save(dir_path)

    def load(self, dir_path):
        self._model = TfEncoder(dir_path)

    def encode(self, data):
        return self._model.encode(data)
