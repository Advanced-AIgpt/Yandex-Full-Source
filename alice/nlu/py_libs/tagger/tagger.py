# coding: utf-8
from __future__ import unicode_literals, print_function

import os
import pickle
import logging
import math
import tempfile
import shutil

import numpy as np

import tensorflow as tf

from collections import defaultdict
from functools import partial
from codecs import open
from tensorflow.python.layers import core as layers_core
from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN
from enum import IntEnum
from sklearn.base import BaseEstimator

from vins_models_tf import TfRnnTagger, convert_model_features
from vins_models_tf import convert_model_to_memmapped


logger = logging.getLogger(__name__)


def get_postag_value(tag):
    if len(tag) > 0:
        return tag[0].value
    return 0


def convert_sample_features(sample_features, features_mapping):
    def convert_ner_data(features_sparse_seq, mapping, features_count):
        result = np.zeros((len(features_sparse_seq), features_count))
        for i, feat_set in enumerate(features_sparse_seq):
            for feat in feat_set:
                result[i, mapping(feat.value)] = feat.weight
        return result

    def build_get_index(mapping, default_index):
        def _get_index(value):
            return mapping[value] if value in mapping else default_index
        return _get_index

    converted_data = []
    converted_data.append(np.array(sample_features.dense_seq['alice_requests_emb']))

    features_sparse_seq = sample_features.sparse_seq

    postag_mapping = build_get_index(features_mapping['postag'], 0)
    converted_data.append(np.array([postag_mapping(get_postag_value(tag)) for tag in features_sparse_seq['postag']]))

    for feature in ['ner', 'wizard']:
        ner_mapping = build_get_index(features_mapping[feature], 0)
        if feature not in features_mapping or not features_mapping[feature]:
            ner_count = 1
        else:
            ner_count = max(features_mapping[feature].values()) + 1
        if feature not in features_sparse_seq:
            converted_data.append(np.zeros((converted_data[0].shape[0], ner_count)))
        else:
            converted_data.append(convert_ner_data(features_sparse_seq[feature], ner_mapping, ner_count))

    return converted_data


class BatchGenerator(object):
    def __init__(self, data, features_mapping, batch_size):
        self._features_mapping = features_mapping
        self._batch_size = batch_size
        self._batches_count = int(math.ceil(float(len(data)) / self._batch_size))
        self._generator = self._iterate_batches(data)

    def _iterate_batches(self, data):
        indices = np.arange(len(data))

        while True:
            np.random.shuffle(indices)

            for i in range(self._batches_count):
                batch_begin = i * self._batch_size
                batch_end = min((i + 1) * self._batch_size, len(data))
                batch_indices = indices[batch_begin: batch_end]

                yield self._build_batch(data, batch_indices)

    def _build_batch(self, data, batch_indices):
        batch_data, batch_labels, batch_seq_lengths = [], [], []
        max_sent_len = 0
        for ind in batch_indices:
            sample_features = data[ind]
            batch_data.append(convert_sample_features(sample_features, self._features_mapping))
            batch_labels.append(np.array([self._features_mapping['label'][tag] for tag in sample_features.sample.tags]))
            batch_seq_lengths.append(len(sample_features))
            max_sent_len = max(max_sent_len, len(sample_features))

        batch_data = [self._concatenate(batch_data, ind, max_sent_len) for ind in range(len(batch_data[0]))]
        batch_labels = self._concatenate(batch_labels, None, max_sent_len)
        batch_seq_lengths = np.array(batch_seq_lengths)

        return batch_data, batch_labels, batch_seq_lengths

    @staticmethod
    def _concatenate(data, feature_ind, sent_len):
        if feature_ind is not None:
            feature_shape = data[0][feature_ind].shape[1:]
        else:
            feature_shape = data[0].shape[1:]
        res = np.zeros((sent_len, len(data)) + feature_shape)
        for sample_ind, features in enumerate(data):
            vector = features[feature_ind] if feature_ind is not None else features
            res[:len(vector), sample_ind] = vector
        return res

    def get_batches_count(self):
        return self._batches_count

    def __iter__(self):
        return self

    def next(self):
        return self._generator.next()


class RnnTaggerModel(object):
    class ModelMode(IntEnum):
        TRAIN = 0
        VAL = 1

    def __init__(self, mode, word_emb_dim=300, postag_emb_dim=16, ner_emb_dim=32, rnn_encoder_dim=128,
                 rnn_encoder_layers=2, rnn_decoder_dim=64, label_emb_dim=32, features_dropout_rate=0.7,
                 rnn_dropout_rate=0.8, rnn_recurrent_dropout_rate=0.7, epochs_count=20, batch_count_to_validate=100,
                 batch_size=256, learning_rate=0.001):
        self._mode = mode
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
        self._epochs_count = epochs_count
        self._batch_count_to_validate = batch_count_to_validate
        self._batch_size = batch_size
        self._learning_rate = learning_rate

        self._features_mapping = None

    def fit(self, data, on_validation=None):
        """
        Trains model on data
        :param data: list of SampleFeatures
        """
        self._features_mapping = self._collect_features(data)
        self._build_model(self._features_mapping)

        batch_generator = BatchGenerator(data, self._features_mapping, self._batch_size)

        training_steps = self._epochs_count * batch_generator.get_batches_count()
        logger.info('Epochs num = {}, training steps num = {}'.format(self._epochs_count, training_steps))

        self._processed_batches = 0
        for epoch in range(self._epochs_count):
            early_stopped = self._run_train_epoch(batch_generator, epoch, on_validation)

            if not early_stopped:
                continue

            logger.info('Early stopped {} epoch after {} batches processed overall'.format(
                epoch + 1, self._processed_batches))
            break

    def _collect_features(self, data, feature_types=['ner', 'wizard', 'postag']):
        feature_sets = defaultdict(set)
        for sample_features in data:
            for feature_type in feature_types:
                if feature_type not in sample_features.sparse_seq:
                    continue
                for token_features in sample_features.sparse_seq[feature_type]:
                    feature_sets[feature_type].update([x.value for x in token_features])
            feature_sets['label'].update(sample_features.sample.tags)

        return self._build_mappings(feature_sets)

    @staticmethod
    def _build_mappings(feature_sets):
        features_mapping = defaultdict(partial(defaultdict, int))
        for feature_type in feature_sets:
            for i, feature in enumerate(feature_sets[feature_type]):
                features_mapping[feature_type][feature] = i + 1
        return features_mapping

    def _build_model(self, features_mapping):

        def get_feature_count(feat):
            # add one dim for padding
            return len(features_mapping[feat]) + 1 if feat in features_mapping else 1

        self._labels_count = get_feature_count('label')

        with tf.Graph().as_default() as graph:
            self._encoder_inputs = []
            self._sequence_lengths = tf.placeholder(tf.int32, shape=[None], name='sequence_lengths')
            self._features_dropout = tf.placeholder_with_default(1., shape=[], name='features_dropout')
            self._rnn_out_dropout = tf.placeholder_with_default(1., shape=[], name='rnn_out_dropout')
            self._recurrent_dropout = tf.placeholder_with_default(1., shape=[], name='recurrent_dropout')

            encoder_out = self._add_inputs(get_feature_count)

            for i in range(self._rnn_encoder_layers):
                encoder_out = self._add_rnn_encoder_layer(encoder_out, '_{}'.format(i + 1))
                encoder_out = tf.nn.dropout(encoder_out, self._recurrent_dropout)

            self._encoder_out = encoder_out
            self._add_decoder(self._encoder_out)

            if self._mode == self.ModelMode.TRAIN:
                self._add_loss()
                self._add_optimizer(lr=self._learning_rate, max_grad_norm=5.)

            self._sess = tf.Session(graph=graph)
            self._sess.run(tf.global_variables_initializer())
            self._saver = tf.train.Saver()

    def _add_inputs(self, get_feature_count):
        embeddings = []

        self._encoder_inputs.append(tf.placeholder(tf.float32, shape=[None, None, self._word_emb_dim], name='word'))
        embeddings.append(self._encoder_inputs[-1])

        self._add_embeddings(name='postag', in_dim=get_feature_count('postag'), out_dim=self._postag_emb_dim,
                             inputs_list=self._encoder_inputs, embeddings_list=embeddings)

        self._add_dense_embeddings(name='ner', in_dim=get_feature_count('ner'), out_dim=self._ner_emb_dim,
                                   inputs_list=self._encoder_inputs, embeddings_list=embeddings)

        self._add_dense_embeddings(name='wizard', in_dim=get_feature_count('wizard'), out_dim=self._ner_emb_dim,
                                   inputs_list=self._encoder_inputs, embeddings_list=embeddings)

        self._encoder_inputs.append(tf.placeholder(tf.int32, shape=[None, None], name='label'))

        return tf.nn.dropout(tf.concat(embeddings, axis=-1), self._features_dropout)

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
            batch_size, seq_len, _ = tf.unstack(tf.shape(inputs))

            self._decoder_cell = LSTMBlockFusedCell(num_units=self._rnn_decoder_dim)

            label_embeddings = tf.get_variable(name='label_emb_matrix', dtype=tf.float32,
                                               shape=[self._labels_count + 1, self._label_emb_dim])

            output_layer = layers_core.Dense(self._labels_count, name='output_layer')

            start_labels = tf.fill([tf.shape(inputs)[1]], self._labels_count, name='start_labels')

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
    def labels_count(self):
        return self._labels_count

    @property
    def session(self):
        return self._sess

    def _run_train_epoch(self, batch_generator, epoch, on_validation=None):
        batches_count = batch_generator.get_batches_count()
        cur_total_loss, accuracy = 0, 0.
        accuracy_metric = self._build_accuracy_metric()
        for i, (data, labels, sequence_lengths) in enumerate(batch_generator):
            feed = {
                self._encoder_inputs[-1]: labels,
                self._sequence_lengths: sequence_lengths,
                self._features_dropout: self._features_dropout_rate,
                self._rnn_out_dropout: self._rnn_dropout_rate,
                self._recurrent_dropout: self._rnn_recurrent_dropout_rate
            }
            for k in range(len(data)):
                feed[self._encoder_inputs[k]] = data[k]

            _, loss, preds = self._sess.run([self._optimizer, self._loss, self._labels_pred], feed_dict=feed)

            accuracy = accuracy_metric(y_true=labels, y_pred=preds)
            cur_total_loss += loss

            self._processed_batches += 1

            if on_validation and self._batch_count_to_validate and self._processed_batches % self._batch_count_to_validate == 0:
                if not on_validation():
                    return True

            if i + 1 == batches_count:
                break

        logger.info('Epoch = {} / {}. Train loss = {:.4f}. Accuracy = {:.2%}'.format(
            epoch + 1, self._epochs_count, cur_total_loss / batches_count, accuracy)
        )
        return False

    @staticmethod
    def _build_accuracy_metric():
        counts = [0, 0]

        def _accuracy(y_true, y_pred):
            counts[0] += np.sum(y_true != 0)
            counts[1] += np.sum((y_pred == y_true) & (y_true != 0))

            if counts[0] > 0:
                return float(counts[1]) / counts[0]
            return 0.0
        return _accuracy

    def save(self, dir_path, base_name='model.ckpt'):
        """
        Saves model's info into the dir_path dir
        """
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)

        self._saver.save(self._sess, os.path.join(dir_path, base_name))

        with open(os.path.join(dir_path, 'features_mapping.pkl'), 'wb') as f:
            pickle.dump(obj=self._features_mapping, file=f, protocol=pickle.HIGHEST_PROTOCOL)

    def restore(self, dir_path, base_name='model.ckpt'):
        with open(os.path.join(dir_path, 'features_mapping.pkl'), 'rb') as f:
            self._features_mapping = pickle.load(f)

        self._build_model(self._features_mapping)
        self._saver.restore(self._sess, os.path.join(dir_path, base_name))


class RnnTaggerTrainer(BaseEstimator):
    _MODEL_FEATURES_FILE_NAME = 'model_features.pkl'
    _MODEL_FILE_NAME = 'model.pb'

    def __init__(self, epochs_count=50, batch_count_to_validate=100, batch_size=256, learning_rate=0.001, rnn_encoder_dim=128, **kwargs):
        self._train_model = None
        self._epochs_count = epochs_count
        self._batch_count_to_validate = batch_count_to_validate
        self._batch_size = batch_size
        self._learning_rate = learning_rate
        self._rnn_encoder_dim = rnn_encoder_dim
        # TODO: current model has fixed architecture based on these features, can be refactored later
        self._used_dense_features = ['alice_requests_emb']
        self._used_sparse_features = ['postag', 'ner', 'wizard']
        self._inputs = []

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

            inference_model = RnnTaggerModel(
                mode=RnnTaggerModel.ModelMode.VAL,
                word_emb_dim=self._train_model.word_embedding_dim,
                rnn_encoder_dim=self._rnn_encoder_dim,
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

    def _save_model_features(self, dir_path, model):
        with open(os.path.join(dir_path, self._MODEL_FEATURES_FILE_NAME), 'wb') as f:
            def convert_node_mapping(mapping):
                return {node_name: node.name for node_name, node in mapping.iteritems()}

            pickled_objects = (
                self._used_dense_features,
                self._used_sparse_features,
                self._train_model.features_mapping,
                self._train_model.decoder_dim,
                self._train_model.labels_count,
                convert_node_mapping(model.get_inputs()),
                convert_node_mapping(model.get_outputs()),
            )
            pickle.dump(obj=pickled_objects, file=f, protocol=pickle.HIGHEST_PROTOCOL)

    def fit(self, X, y, on_validation=None, **kwargs):
        embedding_dim = X[0].dense_seq['alice_requests_emb'].shape[1]

        def on_validation_noargs():
            return on_validation(self)

        self._train_model = RnnTaggerModel(
            mode=RnnTaggerModel.ModelMode.TRAIN,
            word_emb_dim=embedding_dim,
            epochs_count=self._epochs_count,
            batch_count_to_validate=self._batch_count_to_validate,
            batch_size=self._batch_size,
            learning_rate=self._learning_rate,
            rnn_encoder_dim=self._rnn_encoder_dim,
        )
        self._train_model.fit(X, on_validation=on_validation_noargs if on_validation else None)

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
