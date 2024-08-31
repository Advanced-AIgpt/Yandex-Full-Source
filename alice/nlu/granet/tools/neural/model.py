# coding: utf-8

import attr
import json
import logging
import math
import os
import pickle
import shutil
import tempfile

import numpy as np
import tensorflow as tf

from collections import defaultdict
from copy import deepcopy
from enum import IntEnum
from functools import partial
from operator import attrgetter
from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN
from tensorflow.python.layers import core as layers_core

from vins_models_tf import TfRnnTagger, convert_model_features
from vins_models_tf import convert_model_to_memmapped


logger = logging.getLogger(__name__)


@attr.s
class ModelConfig(object):
    tag_count = attr.ib()
    class_count = attr.ib(default=2)
    word_emb_dim = attr.ib(default=300)
    rnn_encoder_dim = attr.ib(default=128)
    rnn_encoder_layers = attr.ib(default=2)
    use_unconditional_predictor = attr.ib(default=False)
    rnn_decoder_dim = attr.ib(default=64)
    label_emb_dim = attr.ib(default=32)
    class_predictor_hidden_dim = attr.ib(default=128)
    features_dropout_rate = attr.ib(default=0.7)
    rnn_dropout_rate = attr.ib(default=0.8)
    rnn_recurrent_dropout_rate = attr.ib(default=0.7)
    learning_rate = attr.ib(default=0.001)
    max_grad_norm = attr.ib(default=5)
    epoch_count = attr.ib(default=20)
    empty_tag_index = attr.ib(default=-1)


class BatchGenerator(object):
    def __init__(self, examples, batch_size):
        self._examples = np.array(examples)
        self._batch_size = batch_size
        self._batches_count = int(math.ceil(float(len(self._examples)) / self._batch_size))
        self._generator = self._iterate_batches(self._examples)

    def _iterate_batches(self, examples):
        indices = np.arange(len(examples))

        while True:
            np.random.shuffle(indices)

            for i in xrange(self._batches_count):
                batch_begin = i * self._batch_size
                batch_end = min((i + 1) * self._batch_size, len(examples))
                batch_indices = indices[batch_begin: batch_end]

                yield self._build_batch(examples[batch_indices])

    def _build_batch(self, samples):
        def batchify_matrix(get_field, max_length, dtype):
            tensor = np.zeros((max_length, len(samples)), dtype=dtype)

            for sample_id, sample in enumerate(samples):
                data = get_field(sample)
                tensor[:len(data), sample_id] = data

            return tensor

        lengths = np.array([len(sample) for sample in samples])
        max_length = max(lengths)

        batch = {
            'lengths': lengths,
            'token_ids': batchify_matrix(attrgetter('token_ids'), max_length, np.int64),
            'tag_ids': batchify_matrix(attrgetter('tag_ids'), max_length + 1, np.int64),
            'tag_mask': batchify_matrix(attrgetter('tag_mask'), max_length, np.int64),
            'class_ids': np.array([sample.class_id for sample in samples])
        }

        return batch

    def __len__(self):
        return self._batches_count

    def __iter__(self):
        return self

    def next(self):
        return self._generator.next()


class RnnTaggerModel(object):
    class ModelMode(IntEnum):
        TRAIN = 0
        VAL = 1

    def __init__(self, mode, config, embeddings_matrix=None):
        self._mode = mode
        self._embeddings_matrix = embeddings_matrix
        self._config = config

    def fit(self, batch_generator):
        self._build_model()

        try:
            for epoch in xrange(self._config.epoch_count):
                self._run_train_epoch(batch_generator, epoch)
        except KeyboardInterrupt:
            pass

    def _build_model(self):
        with tf.Graph().as_default() as graph:
            self._encoder_inputs = []
            self._sequence_lengths = tf.placeholder(tf.int32, shape=[None], name='sequence_lengths')
            self._features_dropout = tf.placeholder_with_default(1., shape=[], name='features_dropout')
            self._rnn_out_dropout = tf.placeholder_with_default(1., shape=[], name='rnn_out_dropout')
            self._recurrent_dropout = tf.placeholder_with_default(1., shape=[], name='recurrent_dropout')

            encoder_out = self._add_inputs()

            encoder_final_state = None
            for i in xrange(self._config.rnn_encoder_layers):
                encoder_out, encoder_final_state = self._add_rnn_encoder_layer(encoder_out, '_{}'.format(i + 1))
                encoder_out = tf.nn.dropout(encoder_out, self._recurrent_dropout)

            self._encoder_out, self._encoder_final_state = encoder_out, encoder_final_state
            if self._config.use_unconditional_predictor:
                self._add_unconditional_predictor(encoder_out, encoder_final_state)
            else:
                self._add_decoder(encoder_out, encoder_final_state)

            if self._mode == self.ModelMode.TRAIN:
                self._add_loss()
                self._add_optimizer(lr=self._config.learning_rate, max_grad_norm=self._config.max_grad_norm)

            self._init_session(graph)
            self._init_saver(graph)

    def _add_inputs(self):
        if self._mode != self.ModelMode.TRAIN:
            with tf.variable_scope('inference'):
                self._words = tf.placeholder(
                    tf.float32, shape=[None, None, self._config.word_emb_dim], name='word_embs'
                )
                rnn_inputs = self._words
        else:
            with tf.variable_scope('train'):
                self._words = tf.placeholder(tf.int32, shape=[None, None], name='words')

                frozen_embeddings_var = tf.Variable(
                    initial_value=self._embeddings_matrix, dtype=tf.float32,
                    trainable=False, name='embedding_matrix'
                )
                rnn_inputs = tf.nn.embedding_lookup(frozen_embeddings_var, self._words)

        self._encoder_inputs.append(self._words)
        return tf.nn.dropout(rnn_inputs, self._features_dropout)

    @staticmethod
    def _add_dense_embeddings(name, in_dim, out_dim, inputs_list, embeddings_list):
        inputs_list.append(tf.placeholder(tf.float32, shape=[None, None, in_dim], name=name))
        with tf.variable_scope(name):
            embeddings_list.append(tf.layers.dense(inputs_list[-1], out_dim, activation=tf.nn.elu))

    @staticmethod
    def _add_embeddings(name, in_dim, out_dim, inputs_list, embeddings_list, embeddings_matrix=None):
        inputs_list.append(tf.placeholder(tf.int32, shape=[None, None], name=name))
        with tf.variable_scope(name):
            if embeddings_matrix is None:
                _embeddings = tf.get_variable(name='emb_matrix', dtype=tf.float32,
                                              shape=[in_dim, out_dim])
            else:
                _embeddings = tf.Variable(embeddings_matrix, name='emb_matrix',
                                          dtype=tf.float32, trainable=False)
            embeddings_list.append(tf.nn.embedding_lookup(_embeddings, inputs_list[-1], name='emb'))

    def _add_rnn_encoder_layer(self, inputs, name_suffix):
        with tf.variable_scope('bi-gru' + name_suffix):
            with tf.variable_scope('forward') as forward_scope:
                cell = LSTMBlockFusedCell(num_units=self._config.rnn_encoder_dim)
                output_fw, final_state_fw = cell(
                    inputs, sequence_length=self._sequence_lengths,
                    dtype=tf.float32, scope=forward_scope
                )

            with tf.variable_scope('backward') as forward_scope:
                cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self._config.rnn_encoder_dim))
                output_bw, final_state_bw = cell(
                    inputs, sequence_length=self._sequence_lengths,
                    dtype=tf.float32, scope=forward_scope
                )

            output = tf.concat([output_fw, output_bw], axis=-1, name='output')
            final_state = tf.concat([final_state_fw.h, final_state_bw.h], axis=-1, name='final_state')
            return output, final_state

    def _add_unconditional_predictor(self, inputs, encoder_final_state):
        tag_output_layer = layers_core.Dense(self._config.tag_count, name='tag_output_layer')

        self._tag_logits = tag_output_layer(inputs)
        self._class_logits = self._add_mlp_class_prediction(encoder_final_state)

        if self._mode == self.ModelMode.TRAIN:
            self._tag_labels = tf.placeholder(tf.int32, shape=[None, None], name='tag_labels')
            self._class_labels = tf.placeholder(dtype=tf.int32, shape=[None], name='class_labels')
        else:
            tag_pred_logprobs = tf.nn.log_softmax(self._tag_logits, axis=-1)
            class_pred_logprobs = tf.nn.log_softmax(self._class_logits, axis=-1)

            self._tag_pred_logprobs = tf.reduce_max(tag_pred_logprobs, axis=-1)
            self._class_pred_logprobs = tf.reduce_max(class_pred_logprobs, axis=-1)

        self._tag_preds = tf.argmax(self._tag_logits, axis=-1)
        self._class_preds = tf.argmax(self._class_logits, axis=-1)

    def _add_decoder(self, inputs, encoder_final_state):
        with tf.variable_scope('decoder') as decoder_scope:
            self._decoder_cell = LSTMBlockFusedCell(num_units=self._config.rnn_decoder_dim)

            tag_embeddings = tf.get_variable(name='label_emb_matrix', dtype=tf.float32,
                                             shape=[self._config.tag_count, self._config.label_emb_dim])

            tag_output_layer = layers_core.Dense(self._config.tag_count, name='tag_output_layer')

            self._class_logits = self._add_mlp_class_prediction(encoder_final_state)

            if self._mode == self.ModelMode.TRAIN:
                self._tag_labels = tf.placeholder(tf.int32, shape=[None, None], name='tag_labels')
                self._class_labels = tf.placeholder(dtype=tf.int32, shape=[None], name='class_labels')
                self._class_preds = tf.argmax(self._class_logits, axis=-1)

                self._add_train_decoder(
                    inputs, tag_embeddings, self._class_labels, tag_output_layer, decoder_scope
                )
            else:
                self._class_preds = tf.nn.log_softmax(self._class_logits)
                self._add_val_decoder(tag_embeddings, tag_output_layer, decoder_scope)

    def _add_train_decoder(self, inputs, tag_embeddings, start_labels, output_layer, decoder_scope):
        prev_labels = self._tag_labels[:-1]
        prev_labels_emb = tf.nn.embedding_lookup(tag_embeddings, prev_labels)

        inputs = tf.concat([inputs, prev_labels_emb], axis=-1, name='decoder_input')
        outputs, _ = self._decoder_cell(inputs=inputs, sequence_length=self._sequence_lengths,
                                        scope=decoder_scope, dtype=tf.float32)

        self._tag_logits = output_layer(outputs)
        self._tag_preds = tf.argmax(self._tag_logits, axis=-1)

    def _add_val_decoder(self, tag_embeddings, output_layer, decoder_scope):
        self._prev_labels_input = tf.placeholder(dtype=tf.int32, shape=[None], name='prev_labels_input')
        prev_labels_emb = tf.nn.embedding_lookup(tag_embeddings, self._prev_labels_input)

        self._encoder_res_input = tf.placeholder(dtype=tf.float32, shape=[self._config.rnn_encoder_dim * 2],
                                                 name='encoder_res_input')

        self._decoder_cell_state_input = tf.placeholder(dtype=tf.float32, shape=[None, self._config.rnn_decoder_dim],
                                                        name='decoder_cell_state_input')
        self._decoder_state_input = tf.placeholder(dtype=tf.float32, shape=[None, self._config.rnn_decoder_dim],
                                                   name='decoder_state_input')

        encoder_res = tf.expand_dims(self._encoder_res_input, axis=0)
        encoder_res = tf.tile(encoder_res, [tf.shape(prev_labels_emb)[0], 1])

        decoder_input = tf.concat([encoder_res, prev_labels_emb], axis=-1, name='decoder_input')
        decoder_input = tf.expand_dims(decoder_input, axis=0)

        sequence_length = tf.ones_like(self._prev_labels_input)

        outputs, (self._decoder_cell_state, self._decoder_state) = self._decoder_cell(
            inputs=decoder_input, sequence_length=sequence_length, scope=decoder_scope,
            initial_state=(self._decoder_cell_state_input, self._decoder_state_input)
        )

        self._tag_preds = tf.nn.log_softmax(output_layer(outputs))

    def _add_mlp_class_prediction(self, inputs):
        class_prediction_layers = []
        if self._config.class_predictor_hidden_dim > 0:
            class_prediction_layers.append(
                layers_core.Dense(128, activation=tf.nn.relu, name='class_output_layer_proj')
            )

        class_prediction_layers.append(layers_core.Dense(self._config.class_count, name='class_output_layer'))

        for layer in class_prediction_layers:
            inputs = layer(inputs)
        return inputs

    def _add_loss(self):
        with tf.variable_scope("loss"):
            self._tag_mask = tf.placeholder(tf.int32, shape=[None, None], name='tag_mask')

            tag_loss = tf.nn.sparse_softmax_cross_entropy_with_logits(
                labels=self._tag_labels[1:],
                logits=self._tag_logits,
                name='tag_loss'
            )
            ones_count = tf.cast(tf.reduce_sum(self._tag_mask), tf.float32)
            self._tag_loss = tf.cond(
                tf.not_equal(ones_count, 0.),
                lambda: tf.reduce_sum(tag_loss) / ones_count,
                lambda: 0.
            )

            class_loss = tf.nn.sparse_softmax_cross_entropy_with_logits(
                labels=self._class_labels,
                logits=self._class_logits,
                name='class_loss'
            )
            self._class_loss = tf.reduce_mean(class_loss)

            self._loss = self._tag_loss + self._class_loss

    def _add_optimizer(self, lr, max_grad_norm=-1.):
        with tf.variable_scope("optimizer"):
            optimizer = tf.train.AdamOptimizer(learning_rate=lr)

            if max_grad_norm > 0.:
                grads, variables = zip(*optimizer.compute_gradients(self._loss))
                grads, global_norm = tf.clip_by_global_norm(grads, max_grad_norm)
                self._optimizer = optimizer.apply_gradients(zip(grads, variables))
            else:
                self._optimizer = optimizer.minimize(self._loss)

    def _init_session(self, graph):
        config = tf.ConfigProto()
        config.gpu_options.allow_growth = True
        self._sess = tf.Session(config=config, graph=graph)
        self._sess.run(tf.global_variables_initializer())

    def _init_saver(self, graph):
        saveable_vars = (
            graph.get_collection(tf.GraphKeys.GLOBAL_VARIABLES)
            + graph.get_collection(tf.GraphKeys.SAVEABLE_OBJECTS)
        )
        saveable_vars = [variable for variable in saveable_vars if not 'train' in variable.name]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    def get_inputs(self):
        nodes = {}
        if self._mode == self.ModelMode.TRAIN:
            nodes = {
                'features_dropout': self._features_dropout,
                'rnn_out_dropout': self._rnn_out_dropout,
                'recurrent_dropout': self._recurrent_dropout,
                'tag_labels': self._tag_labels,
                'class_labels': self._class_labels
            }
        elif not self._config.use_unconditional_predictor:
            nodes = {
                'prev_labels': self._prev_labels_input,
                'encoder_res': self._encoder_res_input,
                'decoder_cell_state': self._decoder_cell_state_input,
                'decoder_state': self._decoder_state_input
            }

        nodes['sequence_lengths'] = self._sequence_lengths
        for i, inp in enumerate(self._encoder_inputs):
            nodes['encoder_input_' + str(i)] = inp

        return nodes

    def get_outputs(self):
        if self._mode == self.ModelMode.TRAIN:
            return {
                'class_logits': self._class_logits,
                'tag_logits': self._tag_logits,
                'class_preds': self._class_preds,
                'tag_preds': self._tag_preds,
            }
        elif self._config.use_unconditional_predictor:
            return {
                'tag_preds': self._tag_preds,
                'class_preds': self._class_preds,
                'tag_pred_logprobs': self._tag_pred_logprobs,
                'class_pred_logprobs': self._class_pred_logprobs,
            }
        else:
            return {
                'preds': self._tag_preds,
                'class_preds': self._class_preds,
                'encoder_out': self._encoder_out,
                'decoder_cell_state': self._decoder_cell_state,
                'decoder_state': self._decoder_state
            }

    @property
    def word_embedding_dim(self):
        return self._config.word_emb_dim

    @property
    def decoder_dim(self):
        return self._config.rnn_decoder_dim

    @property
    def session(self):
        return self._sess

    def _run_train_epoch(self, batch_generator, epoch):
        batches_count = len(batch_generator)
        cur_tag_loss, cur_class_loss = 0., 0.
        tag_accuracy, non_empty_tag_accuracy, class_f1_score = 0., 0., 0.
        tag_accuracy_metric = self._build_accuracy_metric()
        non_empty_tag_accuracy_metric = self._build_accuracy_metric(masked_values=[0, self._config.empty_tag_index])
        class_f1_metric = self._build_f1_metric()

        for i, batch in enumerate(batch_generator):
            assert np.all(batch['token_ids'] < self._embeddings_matrix.shape[0])
            assert np.all(batch['tag_ids'] < self._config.tag_count)
            assert np.all(batch['class_ids'] < self._config.class_count)

            feed = {
                self._encoder_inputs[0]: batch['token_ids'],
                self._tag_labels: batch['tag_ids'],
                self._tag_mask: batch['tag_mask'],
                self._class_labels: batch['class_ids'],
                self._sequence_lengths: batch['lengths'],
                self._features_dropout: self._config.features_dropout_rate,
                self._rnn_out_dropout: self._config.rnn_dropout_rate,
                self._recurrent_dropout: self._config.rnn_recurrent_dropout_rate
            }

            _, _, tag_loss, class_loss, tag_preds, class_preds = self._sess.run(
                [self._optimizer, self._loss, self._tag_loss, self._class_loss,
                 self._tag_preds, self._class_preds], feed_dict=feed
            )

            cur_tag_loss += tag_loss
            cur_class_loss += class_loss
            tag_accuracy = tag_accuracy_metric(y_true=batch['tag_ids'][1:], y_pred=tag_preds)
            non_empty_tag_accuracy = non_empty_tag_accuracy_metric(y_true=batch['tag_ids'][1:], y_pred=tag_preds)
            class_f1 = class_f1_metric(y_true=batch['class_ids'], y_pred=class_preds)

            if i + 1 == batches_count:
                break

        logger.info('Epoch = {} / {}. Tag loss = {:.4f}. Class loss = {:.4f}.'
                    ' Tag accuracy = {:.2%} / {:.2%}. Class F1 = ({:.2%} / {:.2%} / {:.2%})'.format(
            epoch + 1, self._config.epoch_count,
            cur_tag_loss / batches_count, cur_class_loss / batches_count,
            tag_accuracy, non_empty_tag_accuracy, *class_f1
        ))
        return (cur_tag_loss + cur_class_loss) / batches_count

    @staticmethod
    def _build_accuracy_metric(masked_values=[0]):
        counts = [0, 0]

        def _metric(y_true, y_pred):
            assert y_true.shape == y_pred.shape

            mask = np.ones_like(y_true)
            for masked_value in masked_values:
                mask &= y_true != masked_value

            counts[0] += np.sum(mask)
            counts[1] += np.sum((y_pred == y_true) & mask)

            return float(counts[1]) / counts[0] if counts[0] != 0 else 0.
        return _metric

    @staticmethod
    def _build_f1_metric():
        counts = [0., 0., 0.]

        def _metric(y_true, y_pred):
            assert y_true.shape == y_pred.shape

            tp = ((y_pred == 1) * (y_true == 1)).sum()
            fp = ((y_pred == 1) * (y_true == 0)).sum()
            fn = ((y_pred == 0) * (y_true == 1)).sum()

            counts[0] += tp
            counts[1] += fp
            counts[2] += fn

            precision = counts[0] / (counts[0] + counts[1]) if counts[0] + counts[1] != 0 else 0.
            recall = counts[0] / (counts[0] + counts[2]) if counts[0] + counts[2] != 0. else 0.
            f1 = 2 * precision * recall / (precision + recall) if precision + recall != 0. else 0.

            return precision, recall, f1
        return _metric

    def save(self, dir_path, base_name='model.ckpt'):
        """
        Saves model's info into the dir_path dir
        """
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)

        self._saver.save(self._sess, os.path.join(dir_path, base_name))

    def restore(self, dir_path, base_name='model.ckpt'):
        self._build_model()
        self._saver.restore(self._sess, os.path.join(dir_path, base_name))


class RnnTaggerTrainer(object):
    _MODEL_DESCRIPTION_FILE_NAME = 'model_description.json'
    _MODEL_FEATURES_FILE_NAME = 'model_features.pkl'
    _MODEL_FILE_NAME = 'model.pb'

    def __init__(self, embeddings_matrix=None, epoch_count=50, features_mapping=None, **kwargs):
        self._embeddings_matrix = embeddings_matrix
        self._train_model = None
        self._epoch_count = epoch_count
        self._features_mapping = features_mapping

        self._config = ModelConfig(
            tag_count=max(self._features_mapping['tags'].values()) + 1,
            class_count=max(self._features_mapping['classes'].values()) + 1,
            epoch_count=self._epoch_count,
            empty_tag_index=features_mapping['tags']['O']
        )

    def save(self, dir_path, min_conversion_size_bytes=100):
        """
        Saves model's info into the dir_path dir
        """
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        assert self._train_model, 'You should fit trainer before saving model'
        self._save_trained_model(dir_path, min_conversion_size_bytes)

    def _save_trained_model(self, dir_path, min_conversion_size_bytes):
        inference_model = self._convert_model_for_inference()

        outputs = [out_node.op.name for out_name, out_node in inference_model.get_outputs().iteritems()]
        self._save_frozen_graph(dir_path, inference_model.session, outputs, min_conversion_size_bytes)
        self._save_model_features(dir_path, inference_model)
        convert_model_features(dir_path)

    def _convert_model_for_inference(self):
        temp_dir = None
        try:
            temp_dir = tempfile.mkdtemp('tagger_model_tmp')
            self._train_model.save(temp_dir)

            inference_model = RnnTaggerModel(mode=RnnTaggerModel.ModelMode.VAL, config=self._config)
            inference_model.restore(temp_dir)
        finally:
            if temp_dir:
                shutil.rmtree(temp_dir)
        return inference_model

    def _save_frozen_graph(self, dir_path, sess, outputs, min_conversion_size_bytes):
        graph_def = sess.graph.as_graph_def()
        graph_def = tf.graph_util.convert_variables_to_constants(sess, graph_def, outputs)

        model_path = os.path.join(dir_path, self._MODEL_FILE_NAME)
        with open(model_path, 'wb') as f:
            f.write(graph_def.SerializeToString())
        convert_model_to_memmapped(model_path, min_conversion_size_bytes)

    def _save_model_features(self, dir_path, model):
        with open(os.path.join(dir_path, self._MODEL_FEATURES_FILE_NAME), 'wb') as f:
            def convert_node_mapping(mapping):
                return {node_name: node.name for node_name, node in mapping.iteritems()}

            features_mapping = deepcopy(self._features_mapping)
            features_mapping['label'] = features_mapping['tags']
            del features_mapping['tags']

            pickled_objects = (
                ['alice_requests_emb'],
                [],
                features_mapping,
                self._train_model.decoder_dim,
                self._config.tag_count,
                convert_node_mapping(model.get_inputs()),
                convert_node_mapping(model.get_outputs()),
            )
            pickle.dump(obj=pickled_objects, file=f, protocol=pickle.HIGHEST_PROTOCOL)

    def fit(self, batch_generator, **kwargs):
        self._train_model = RnnTaggerModel(
            mode=RnnTaggerModel.ModelMode.TRAIN, config=self._config,
            embeddings_matrix=self._embeddings_matrix
        )
        self._train_model.fit(batch_generator)

    def convert_to_applier(self):
        tmpdir = None
        try:
            tmpdir = tempfile.mkdtemp()
            self.save(tmpdir)
            return RnnTaggerApplier(tmpdir)
        finally:
            if tmpdir:
                shutil.rmtree(tmpdir)


class RnnTaggerApplier(object):
    def __init__(self, dir_path):
        self._applier = None
        self.load(dir_path)

    def save(self, dir_path):
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        self._applier.save(dir_path)

    def load(self, dir_path):
        self._applier = TfRnnTagger(dir_path)

    def predict(self, data, nbest=10, beam_width=10):
        assert self._applier, 'You have to load model first'

        return self._applier.predict(data, nbest, beam_width)
