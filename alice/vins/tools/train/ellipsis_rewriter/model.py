# coding: utf-8

import json
import logging
import math
import os
import pickle
import shutil
import tempfile

import numpy as np
import tensorflow as tf

from codecs import open
from collections import defaultdict
from enum import IntEnum
from functools import partial
from operator import attrgetter
from sklearn.base import BaseEstimator
from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN
from tensorflow.python.layers import core as layers_core

from vins_models_tf import TfRnnTagger, convert_model_features
from vins_models_tf import convert_model_to_memmapped


logger = logging.getLogger(__name__)


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
        def batchify_matrix(get_field, dtype):
            tensor = np.zeros((max_length, len(samples)), dtype=dtype)

            for sample_id, sample in enumerate(samples):
                data = get_field(sample)
                tensor[:len(data), sample_id] = data

            return tensor

        lengths = np.array([len(sample) for sample in samples])
        max_length = max(lengths)

        batch = {
            'lengths': lengths,
            'token_ids': batchify_matrix(attrgetter('token_ids'), np.int64),
            'tag_ids': batchify_matrix(attrgetter('tag_ids'), np.int64)
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

    def __init__(self, mode, word_emb_dim=300, label_count=3, embeddings_matrix=None,
                 postag_emb_dim=16, ner_emb_dim=32, rnn_encoder_dim=128, rnn_encoder_layers=2,
                 rnn_decoder_dim=64, label_emb_dim=32, features_dropout_rate=0.7,
                 rnn_dropout_rate=0.8, rnn_recurrent_dropout_rate=0.7,
                 spec_token_count=2, epochs_count=20):
        self._mode = mode
        self._label_count = label_count
        self._embeddings_matrix = embeddings_matrix
        self._word_emb_dim = word_emb_dim
        self._postag_emb_dim = postag_emb_dim
        self._ner_emb_dim = ner_emb_dim
        self._rnn_encoder_dim = rnn_encoder_dim
        self._rnn_encoder_layers = rnn_encoder_layers
        self._rnn_decoder_dim = rnn_decoder_dim
        self._label_emb_dim = label_emb_dim
        self._features_dropout_rate = features_dropout_rate
        self._rnn_dropout_rate = rnn_dropout_rate
        self._rnn_recurrent_dropout_rate = rnn_recurrent_dropout_rate
        self._spec_token_count = spec_token_count
        self._epochs_count = epochs_count

        self._features_mapping = {
            'label': {
                'O': 1,
                'sense': 2
            }
        }

    def fit(self, batch_generator):
        """
        Trains model on data
        :param data: list of SampleFeatures
        """
        self._build_model()

        try:
            for epoch in xrange(self._epochs_count):
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

            for i in xrange(self._rnn_encoder_layers):
                encoder_out = self._add_rnn_encoder_layer(encoder_out, '_{}'.format(i + 1))
                encoder_out = tf.nn.dropout(encoder_out, self._recurrent_dropout)

            self._encoder_out = encoder_out
            self._add_decoder(self._encoder_out)

            if self._mode == self.ModelMode.TRAIN:
                self._add_loss()
                self._add_optimizer(lr=0.001, max_grad_norm=5.)

            self._init_session(graph)
            self._init_saver(graph)

    def _add_inputs(self):
        embeddings = []

        if self._mode != self.ModelMode.TRAIN:
            with tf.variable_scope('inference'):
                self._words = tf.placeholder(
                    tf.float32, shape=[None, None, self._word_emb_dim], name='word_embs'
                )
                embeddings.append(self._words)
        else:
            with tf.variable_scope('train'):
                self._words = tf.placeholder(tf.int32, shape=[None, None], name='words')

                frozen_embeddings_var = tf.Variable(
                    initial_value=self._embeddings_matrix, dtype=tf.float32, trainable=False, name='embedding_matrix'
                )
                word_embs = tf.nn.embedding_lookup(frozen_embeddings_var, self._words)

                self._trainable_embeddings_var = tf.get_variable(
                    name='trainable_embeddings',
                    shape=[self._spec_token_count - 1, self._embeddings_matrix.shape[-1]]
                )

                trainable_embeddings = tf.concat(
                    [tf.zeros((1, self._embeddings_matrix.shape[-1])), self._trainable_embeddings_var], 0
                )
                special_symbol_positions = tf.where(
                    self._words < self._spec_token_count, self._words, tf.zeros_like(self._words)
                )
                word_embs += tf.nn.embedding_lookup(trainable_embeddings, special_symbol_positions)

                embeddings.append(word_embs)

                self._word_embs = word_embs

        self._encoder_inputs.append(self._words)
        self._encoder_inputs.append(tf.placeholder(tf.int32, shape=[None, None], name='label'))

        return tf.nn.dropout(embeddings[0], self._features_dropout)

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
                cell = LSTMBlockFusedCell(num_units=self._rnn_encoder_dim)
                output_fw, _ = cell(inputs, sequence_length=self._sequence_lengths,
                                    dtype=tf.float32, scope=forward_scope)

            with tf.variable_scope('backward') as forward_scope:
                cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self._rnn_encoder_dim))
                output_bw, _ = cell(inputs, sequence_length=self._sequence_lengths,
                                    dtype=tf.float32, scope=forward_scope)

            return tf.concat([output_fw, output_bw], axis=-1, name='output')

    def _add_decoder(self, inputs):
        with tf.variable_scope('decoder') as decoder_scope:
            self._decoder_cell = LSTMBlockFusedCell(num_units=self._rnn_decoder_dim)

            label_embeddings = tf.get_variable(name='label_emb_matrix', dtype=tf.float32,
                                               shape=[self._label_count + 1, self._label_emb_dim])

            output_layer = layers_core.Dense(self._label_count, name='output_layer')

            start_labels = tf.fill([tf.shape(inputs)[1]], self._label_count, name='start_labels')

            if self._mode == self.ModelMode.TRAIN:
                self._add_train_decoder(inputs, label_embeddings, start_labels, output_layer, decoder_scope)
            else:
                self._add_val_decoder(label_embeddings, output_layer, decoder_scope)

    def _add_train_decoder(self, inputs, label_embeddings, start_labels, output_layer, decoder_scope):
        labels = self._encoder_inputs[-1]
        prev_labels = tf.concat([tf.expand_dims(start_labels, 0), labels[:-1]], axis=0, name='prev_labels')
        prev_labels_emb = tf.nn.embedding_lookup(label_embeddings, prev_labels)

        inputs = tf.concat([inputs, prev_labels_emb], axis=-1, name='decoder_input')
        outputs, _ = self._decoder_cell(inputs=inputs, sequence_length=self._sequence_lengths,
                                        scope=decoder_scope, dtype=tf.float32)

        self._logits = output_layer(outputs)
        self._labels_pred = tf.argmax(self._logits, axis=-1)

    def _add_val_decoder(self, label_embeddings, output_layer, decoder_scope):
        self._prev_labels_input = tf.placeholder(dtype=tf.int32, shape=[None], name='prev_labels_input')
        prev_labels_emb = tf.nn.embedding_lookup(label_embeddings, self._prev_labels_input)

        self._encoder_res_input = tf.placeholder(dtype=tf.float32, shape=[self._rnn_encoder_dim * 2],
                                                 name='encoder_res_input')

        self._decoder_cell_state_input = tf.placeholder(dtype=tf.float32, shape=[None, self._rnn_decoder_dim],
                                                        name='decoder_cell_state_input')
        self._decoder_state_input = tf.placeholder(dtype=tf.float32, shape=[None, self._rnn_decoder_dim],
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

        self._preds = tf.nn.log_softmax(output_layer(outputs))

    def _add_loss(self):
        with tf.variable_scope("loss"):
            losses = tf.nn.sparse_softmax_cross_entropy_with_logits(labels=self._encoder_inputs[-1],
                                                                    logits=self._logits, name='loss')
            mask = tf.sequence_mask(self._sequence_lengths)
            losses = tf.boolean_mask(losses, mask)
            self._loss = tf.reduce_mean(losses)

    def _add_optimizer(self, lr, max_grad_norm=-1.):
        with tf.variable_scope("optimizer"):
            optimizer = tf.train.AdamOptimizer(lr)

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
            graph.get_collection(tf.GraphKeys.GLOBAL_VARIABLES) + graph.get_collection(tf.GraphKeys.SAVEABLE_OBJECTS)
        )
        saveable_vars = [variable for variable in saveable_vars if not 'train' in variable.name]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    def get_inputs(self):
        if self._mode == self.ModelMode.TRAIN:
            nodes = {
                'features_dropout': self._features_dropout,
                'rnn_out_dropout': self._rnn_out_dropout,
                'recurrent_dropout': self._recurrent_dropout,
                'labels': self._encoder_inputs[-1]
            }
        else:
            nodes = {
                'prev_labels': self._prev_labels_input,
                'encoder_res': self._encoder_res_input,
                'decoder_cell_state': self._decoder_cell_state_input,
                'decoder_state': self._decoder_state_input
            }

        nodes['sequence_lengths'] = self._sequence_lengths
        for i, inp in enumerate(self._encoder_inputs[:-1]):
            nodes['encoder_input_' + str(i)] = inp

        return nodes

    def get_outputs(self):
        if self._mode == self.ModelMode.TRAIN:
            return {
                'logits': self._logits,
                'label_preds': self._labels_pred
            }
        else:
            return {
                'encoder_out': self._encoder_out,
                'preds': self._preds,
                'decoder_cell_state': self._decoder_cell_state,
                'decoder_state': self._decoder_state
            }

    @property
    def features_mapping(self):
        return self._features_mapping

    @property
    def word_embedding_dim(self):
        return self._word_emb_dim

    @property
    def decoder_dim(self):
        return self._rnn_decoder_dim

    @property
    def label_count(self):
        return self._label_count

    @property
    def session(self):
        return self._sess

    def _run_train_epoch(self, batch_generator, epoch):
        batches_count = len(batch_generator)
        cur_total_loss, f1_score = 0, 0.
        f1_metric = self._build_f1_metric(self._features_mapping['label']['sense'])
        for i, batch in enumerate(batch_generator):
            assert np.all(batch['token_ids'] < self._embeddings_matrix.shape[0])

            feed = {
                self._encoder_inputs[0]: batch['token_ids'],
                self._encoder_inputs[1]: batch['tag_ids'],
                self._sequence_lengths: batch['lengths'],
                self._features_dropout: self._features_dropout_rate,
                self._rnn_out_dropout: self._rnn_dropout_rate,
                self._recurrent_dropout: self._rnn_recurrent_dropout_rate
            }

            _, loss, preds = self._sess.run([self._optimizer, self._loss, self._labels_pred], feed_dict=feed)

            f1_score = f1_metric(y_true=batch['tag_ids'], y_pred=preds)
            cur_total_loss += loss

            if i + 1 == batches_count:
                break

        logger.info('Epoch = {} / {}. Train loss = {:.4f}. F1 = ({:.2%} / {:.2%} / {:.2%})'.format(
            epoch + 1, self._epochs_count, cur_total_loss / batches_count, *f1_score)
        )
        return cur_total_loss / batches_count

    @property
    def trainable_embeddings(self):
        return self._sess.run(self._trainable_embeddings_var)

    @staticmethod
    def _build_accuracy_metric():
        counts = [0, 0]

        def _accuracy(y_true, y_pred):
            counts[0] += np.sum(y_true != 0)
            counts[1] += np.sum((y_pred == y_true) & (y_true != 0))

            return float(counts[1]) / counts[0] if counts[0] != 0 else 0.
        return _accuracy

    @staticmethod
    def _build_f1_metric(positive_class):
        counts = [0., 0., 0.]

        def metric(y_true, y_pred):
            assert y_true.shape == y_pred.shape

            tp = ((y_pred == positive_class) * (y_true == positive_class)).sum()
            fp = ((y_pred == positive_class) * (y_true != positive_class)).sum()
            fn = ((y_pred != positive_class) * (y_true == positive_class)).sum()

            counts[0] += tp
            counts[1] += fp
            counts[2] += fn

            precision = counts[0] / (counts[0] + counts[1]) if counts[0] + counts[1] != 0 else 0.
            recall = counts[0] / (counts[0] + counts[2]) if counts[0] + counts[2] != 0. else 0.

            f1 = 2 * precision * recall / (precision + recall) if precision + recall != 0. else 0.

            return precision, recall, f1

        return metric

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


class RnnTaggerTrainer(BaseEstimator):
    _MODEL_FEATURES_FILE_NAME = 'model_features.pkl'
    _MODEL_FILE_NAME = 'model.pb'

    def __init__(self, embeddings_matrix=None, epochs_count=50, **kwargs):
        self._embeddings_matrix = embeddings_matrix
        self._train_model = None
        self._epochs_count = epochs_count

    def save(self, dir_path, min_conversion_size_bytes=100):
        """
        Saves model's info into the dir_path dir
        """
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        assert self._train_model, 'You should fit trainer before saving model'
        self._save_trained_model(dir_path, min_conversion_size_bytes)

    def _save_trained_model(self, dir_path, min_conversion_size_bytes):
        trainable_embeddings = self._train_model.trainable_embeddings

        inference_model = self._convert_model_for_inference()

        outputs = [out_node.op.name for out_name, out_node in inference_model.get_outputs().iteritems()]
        self._save_frozen_graph(dir_path, inference_model.session, outputs, min_conversion_size_bytes)
        self._save_model_features(dir_path, trainable_embeddings, inference_model)

        convert_model_features(dir_path)

    def _convert_model_for_inference(self):
        temp_dir = None
        try:
            temp_dir = tempfile.mkdtemp('tagger_model_tmp')
            self._train_model.save(temp_dir)

            inference_model = RnnTaggerModel(
                mode=RnnTaggerModel.ModelMode.VAL, word_emb_dim=self._train_model.word_embedding_dim
            )
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

    def _save_model_features(self, dir_path, trainable_embeddings, model):
        with open(os.path.join(dir_path, self._MODEL_FEATURES_FILE_NAME), 'wb') as f:
            def convert_node_mapping(mapping):
                return {node_name: node.name for node_name, node in mapping.iteritems()}

            pickled_objects = (
                ['alice_requests_emb'],
                [],
                self._train_model.features_mapping,
                self._train_model.decoder_dim,
                self._train_model.label_count,
                convert_node_mapping(model.get_inputs()),
                convert_node_mapping(model.get_outputs()),
            )
            pickle.dump(obj=pickled_objects, file=f, protocol=pickle.HIGHEST_PROTOCOL)

        np.save(os.path.join(dir_path, 'trainable_embeddings.npy'), trainable_embeddings)

        assert trainable_embeddings.shape[0] == 1
        trainable_embeddings = [float(x) for x in trainable_embeddings[0]]
        with open(os.path.join(dir_path, 'trainable_embeddings.json'), 'w') as f:
            json.dump({'[SEP]': trainable_embeddings}, f, indent=2)

    def fit(self, batch_generator, **kwargs):
        self._train_model = RnnTaggerModel(
            mode=RnnTaggerModel.ModelMode.TRAIN, embeddings_matrix=self._embeddings_matrix,
            epochs_count=self._epochs_count
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
