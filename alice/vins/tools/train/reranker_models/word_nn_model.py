# -*- coding: utf-8 -*-
""" Model trainer for WordNNFactor: simple word-level rnn or conv encoder
"""

import os
import pickle
import math
import tqdm
import numpy as np
import tensorflow as tf

# Imports used only for typing
from typing import Dict, Union, Tuple, List, Generator, AnyStr, NoReturn  # noqa: F401
from dataset import VinsDatasetView  # noqa: F401

from collections import defaultdict

from sklearn.utils.multiclass import unique_labels
from sklearn.metrics import classification_report, f1_score, accuracy_score
from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN

from vins_models_tf import save_encoder_description, save_serialized_model_as_memmapped


class BatchGenerator(object):
    def __init__(self, dataset, batch_size, shuffle=False, batch_first=False):
        # type: (VinsDatasetView, int, bool, bool) -> NoReturn

        self._dataset = dataset
        self._batch_size = batch_size
        self._shuffle = shuffle
        self._concat_features = self._concat_features_batch_first if batch_first else self._concat_features_seq_first

    def get_batches_count(self):
        # type: () -> int
        return int(math.ceil(float(len(self._dataset)) / self._batch_size))

    def _iterate_batches(self):
        # type: () -> Generator[Tuple[Dict[AnyStr, np.ndarray], Union[np.ndarray, None], List[int]], None, None]

        indices = np.arange(len(self._dataset))
        batches_count = self.get_batches_count()

        if self._shuffle:
            np.random.shuffle(indices)

        for i in xrange(batches_count):
            batch_begin = i * self._batch_size
            batch_end = min((i + 1) * self._batch_size, len(self._dataset))
            batch_indices = indices[batch_begin: batch_end]

            batch_data = self._dataset[batch_indices]
            batch_labels = batch_data.get('intent', None)

            if batch_labels is not None:
                batch_data.pop('intent')

            data = batch_data.itervalues().next()
            batch_seq_lengths = [len(feat) for feat in data]
            max_seq_len = max(batch_seq_lengths)

            batch_data = {
                feat_type: self._concat_features(max_seq_len, feat_list)
                for feat_type, feat_list in batch_data.iteritems()
            }

            yield batch_data, batch_labels, batch_seq_lengths

    @staticmethod
    def _concat_features_batch_first(max_seq_len, feat_list):
        # type: (int, List[np.ndarray]) -> np.ndarray

        feat_matrix = np.zeros((len(feat_list), max_seq_len) + feat_list[0].shape[1:])
        for i, feat in enumerate(feat_list):
            feat_matrix[i, :feat.shape[0]] = feat
        return feat_matrix

    @staticmethod
    def _concat_features_seq_first(max_seq_len, feat_list):
        # type: (int, List[np.ndarray]) -> np.ndarray

        feat_matrix = np.zeros((max_seq_len, len(feat_list)) + feat_list[0].shape[1:])
        for i, feat in enumerate(feat_list):
            feat_matrix[:feat.shape[0], i] = feat
        return feat_matrix

    def __iter__(self):
        return self._iterate_batches()


class BaseWordNNModel(object):
    _CLASSIFIER_NAME = 'word_nn'
    _BATCH_FIRST = True
    _EMBEDDINGS_IDS_NAME = 'emb_ids'
    _EMBEDDINGS_NAME = 'alice_requests_emb'
    _MODEL_NAME = 'model'
    _MODEL_INFO_SUFFIX = '_info.pkl'
    _MODEL_CHECKPOINT_SUFFIX = '.ckpt'
    _MODEL_MEMMAP_SUFFIX = '.mmap'
    _MODEL_LABELS_SUFFIX = '.labels'

    def __init__(self, word_emb_dim=None, embeddings_matrix=None, label_names=None,
                 encoder_dim=256, dropout_rate=0.3, is_train_model=True):
        self._word_emb_dim = embeddings_matrix.shape[1] if embeddings_matrix is not None else word_emb_dim
        self._embeddings_matrix = embeddings_matrix
        self._label_names = label_names
        self._label_to_index = {label: ind for ind, label in enumerate(label_names)} if label_names else None
        self._is_train_model = is_train_model

        self._encoder_dim = encoder_dim
        self._dropout_rate = dropout_rate

        self._train_batch_generator = None
        self._val_batch_generator = None

        self._is_training = None
        self._sequence_lengths = None
        self._words = None
        self._labels = None
        self._reset_ops = None
        self._accuracy = None
        self._preds = None
        self._loss = None
        self._train_step = None
        self._sess = None

        if self._can_build_model():
            # Build model only in case when all shapes are known
            self._build_model()

    @classmethod
    def train_feature_list(cls):
        # type: () -> List[AnyStr]
        return [cls._EMBEDDINGS_IDS_NAME]

    @classmethod
    def inference_feature_list(cls):
        # type: () -> List[AnyStr]
        return [cls._EMBEDDINGS_NAME]

    @classmethod
    def classifier_name(cls):
        # type: () -> AnyStr
        return cls._CLASSIFIER_NAME

    @property
    def classes(self):
        return self._label_names

    def fit(self, train_dataset, val_dataset=None, train_batch_size=128, val_batch_size=1024, epochs_count=100):
        # type: (VinsDatasetView, VinsDatasetView, int, int, int) -> NoReturn

        self._label_names = ['<unk>'] + train_dataset.possible_intents
        self._label_to_index = {label: ind for ind, label in enumerate(self._label_names)}

        if not self._word_emb_dim:
            self._word_emb_dim = train_dataset.get_features()[self._EMBEDDINGS_NAME]

        self._train_batch_generator = BatchGenerator(
            train_dataset, shuffle=True, batch_size=train_batch_size, batch_first=self._BATCH_FIRST
        )
        if val_dataset:
            self._val_batch_generator = BatchGenerator(
                val_dataset, batch_size=val_batch_size, batch_first=self._BATCH_FIRST
            )
        else:
            self._val_batch_generator = None

        self._build_model()

        for epoch in xrange(epochs_count):
            self._run_epoch(epoch, epochs_count, batch_generator=self._train_batch_generator, is_train_epoch=True)
            if self._val_batch_generator:
                self._run_epoch(epoch, epochs_count, batch_generator=self._val_batch_generator, is_train_epoch=False)

    def _can_build_model(self):
        return self._word_emb_dim is not None and self._label_names is not None

    def _build_model(self):
        with tf.Graph().as_default() as graph:
            self._is_training = tf.placeholder_with_default(False, shape=(), name='training')
            self._sequence_lengths = tf.placeholder(tf.int32, shape=[None], name='sequence_lengths')
            self._labels = tf.placeholder(tf.int64, shape=[None], name='labels')
            self._reset_ops = []

            inputs = self._add_inputs()
            inputs = self._add_encoder(inputs)
            logits = self._add_outputs(inputs)

            if self._is_train_model:
                self._add_loss(logits)
                self._add_accuracy()
                self._add_optimizer()

            self._init_session(graph)
            self._init_saver(graph)

    def _add_inputs(self):
        if self._embeddings_matrix is not None:
            with tf.variable_scope('train'):
                self._words = tf.placeholder(tf.int32, shape=[None, None], name='words')

                embeddings_matrix_var = tf.Variable(initial_value=self._embeddings_matrix, dtype=tf.float32,
                                                    trainable=False, name='embedding_matrix')

                return tf.nn.embedding_lookup(embeddings_matrix_var, self._words)
        else:
            with tf.variable_scope('inference'):
                self._words = tf.placeholder(tf.float32, shape=[None, None, self._word_emb_dim], name='word_embs')
                return self._words

    def _add_encoder(self, inputs):
        raise NotImplementedError()

    def _add_outputs(self, inputs):
        with tf.variable_scope('output'):
            logits = tf.layers.dense(inputs, len(self._label_names))
            self._preds = tf.argmax(logits, axis=-1)
            self._pred_probas = tf.nn.softmax(logits, dim=-1)
            return logits

    def _add_loss(self, logits):
        with tf.variable_scope('loss'):
            losses = tf.nn.sparse_softmax_cross_entropy_with_logits(
                labels=self._labels, logits=logits, name='loss'
            )
            self._loss = tf.reduce_mean(losses)

    def _add_accuracy(self):
        with tf.variable_scope('accuracy'):
            correct_count = tf.reduce_sum(tf.cast(tf.equal(self._preds, self._labels), dtype=tf.int32))
            total_count = tf.shape(self._preds)[0]
            self._accuracy = tf.cond(
                tf.not_equal(total_count, 0),
                lambda: tf.truediv(correct_count, total_count),
                lambda: tf.constant(0., dtype=tf.float64)
            )

    def _add_optimizer(self):
        with tf.variable_scope('optimizer'):
            optimizer = tf.train.AdamOptimizer()

            grads, variables = zip(*optimizer.compute_gradients(self._loss))
            grads, global_norm = tf.clip_by_global_norm(grads, 5.)
            self._train_step = optimizer.apply_gradients(zip(grads, variables))

    def _init_session(self, graph):
        config = tf.ConfigProto()
        config.gpu_options.allow_growth = True
        self._sess = tf.Session(config=config, graph=graph)
        self._sess.run(tf.global_variables_initializer())

    def _init_saver(self, graph):
        saveable_vars = (
            graph.get_collection(tf.GraphKeys.GLOBAL_VARIABLES) + graph.get_collection(tf.GraphKeys.SAVEABLE_OBJECTS)
        )
        saveable_vars = [variable for variable in saveable_vars if not variable.name.startswith('train')]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    def _run_epoch(self, epoch, epochs_count, batch_generator, is_train_epoch):
        batches_count = batch_generator.get_batches_count()
        total_loss, accuracy = 0., 0.
        total_labels, total_preds = [], []
        name = 'Train' if is_train_epoch else 'Val'

        train_step = [self._train_step] if is_train_epoch else []
        with tqdm.tqdm(total=batches_count) as progress_bar:
            for data, labels, sequence_lengths in batch_generator:
                labels = np.array([self._label_to_index.get(label, 0) for label in labels])

                feed = {
                    self._words: data[self._EMBEDDINGS_IDS_NAME].squeeze(-1),
                    self._labels: labels,
                    self._is_training: is_train_epoch,
                    self._sequence_lengths: sequence_lengths,
                }

                loss, accuracy, preds = self._sess.run(
                    [self._loss, self._accuracy, self._preds] + train_step, feed_dict=feed
                )[:3]
                total_loss += loss

                total_labels.append(labels)
                total_preds.append(preds)

                progress_bar.update()
                progress_bar.set_description(
                    '[{} / {}] {:>5s}: Loss = {:.4f}. Accuracy = {:.2%}'.format(
                        epoch + 1, epochs_count, name, loss, accuracy
                    )
                )

            total_labels = np.concatenate(total_labels, axis=-1)
            total_preds = np.concatenate(total_preds, axis=-1)
            progress_bar.set_description(
                '[{} / {}] {:>5s}: Total loss = {:.4f}. Total accuracy = {:.2%}, F1 = {:.2%}'.format(
                    epoch + 1, epochs_count, name, total_loss / batches_count,
                    accuracy_score(total_labels, total_preds),
                    f1_score(total_labels, total_preds, average='weighted')
                )
            )
            progress_bar.refresh()

        if epoch != 0 and epoch % 5 == 0 and not is_train_epoch:
            labels = unique_labels(total_labels, total_preds)
            label_names = [self._label_names[ind] for ind in labels]
            print classification_report(total_labels, total_preds, labels=labels, target_names=label_names, digits=4)

        return total_loss / batches_count

    def predict_proba(self, dataset, batch_size=1024):
        # type: (VinsDatasetView, int) -> np.ndarray

        batch_generator = BatchGenerator(
            dataset, batch_size=batch_size, batch_first=self._BATCH_FIRST
        )
        preds = np.zeros((len(dataset), len(self._label_names)))

        cur_pos = 0
        for data, _, sequence_lengths in batch_generator:
            feed = {
                self._words: data[self._EMBEDDINGS_NAME],
                self._sequence_lengths: sequence_lengths
            }
            cur_preds = self._sess.run([self._pred_probas], feed_dict=feed)[0]
            preds[cur_pos: cur_pos + len(sequence_lengths), :] = cur_preds
            cur_pos += len(sequence_lengths)

        return preds

    def predict(self, dataset, batch_size=1024):
        # type: (VinsDatasetView, int) -> List[AnyStr]

        probs = self.predict_proba(dataset, batch_size)
        preds = probs.argmax(axis=-1)

        return [self._label_names[ind] for ind in preds]

    def save(self, dir_path, base_name=None):
        # type: (AnyStr, AnyStr) -> None

        base_name = base_name or self._MODEL_NAME + self._MODEL_CHECKPOINT_SUFFIX

        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        with open(os.path.join(dir_path, self._MODEL_NAME + self._MODEL_INFO_SUFFIX), 'wb') as f:
            pickle.dump((self._word_emb_dim, self._label_names, self._encoder_dim), f, pickle.HIGHEST_PROTOCOL)

        self._saver.save(self._sess, os.path.join(dir_path, base_name), write_meta_graph=False)

    def save_frozen_graph(self, dir_path, normalize_intent_name, min_conversion_size_bytes=100):
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        graph_def = self._sess.graph.as_graph_def()
        graph_def = tf.graph_util.convert_variables_to_constants(
            self._sess, graph_def, [self._pred_probas.name.split(':')[0]]
        )

        save_serialized_model_as_memmapped(
            os.path.join(dir_path, self._MODEL_NAME + self._MODEL_MEMMAP_SUFFIX),
            graph_def.SerializeToString(),
            min_conversion_size_bytes
        )

        inputs = {'dense_seq': [self._words.name]}
        save_encoder_description(dir_path, (inputs, self._pred_probas.name))

        label_to_indices = defaultdict(list)
        for i, label in enumerate(self._label_names):
            label_to_indices[normalize_intent_name(label)].append(i)

        with open(os.path.join(dir_path, self._MODEL_NAME + self._MODEL_LABELS_SUFFIX), 'wb') as f:
            pickle.dump((label_to_indices, len(self._label_names)), file=f, protocol=pickle.HIGHEST_PROTOCOL)

    @classmethod
    def restore(cls, model_path, base_name=None):
        # type: (AnyStr, AnyStr) -> BaseWordNNModel

        base_name = base_name or (cls._MODEL_NAME + cls._MODEL_CHECKPOINT_SUFFIX)

        with open(os.path.join(model_path, cls._MODEL_NAME + cls._MODEL_INFO_SUFFIX), 'rb') as f:
            word_emb_dim, label_names, encoder_dim = pickle.load(f)

        model = cls(word_emb_dim=word_emb_dim, label_names=label_names, encoder_dim=encoder_dim, is_train_model=False)
        assert model._can_build_model()
        model._saver.restore(model._sess, os.path.join(model_path, base_name))

        return model


class ConvWordNNModel(BaseWordNNModel):
    _CLASSIFIER_NAME = 'conv'
    _BATCH_FIRST = True

    def __init__(self, kernel_size=3, **kwargs):
        super(ConvWordNNModel, self).__init__(**kwargs)

        self._kernel_size = kernel_size

    def _add_encoder(self, inputs):
        with tf.variable_scope('encoder'):
            if self._is_train_model:
                inputs = tf.layers.dropout(inputs, rate=self._dropout_rate, training=self._is_training)
            inputs = tf.layers.conv1d(
                inputs, self._encoder_dim, kernel_size=self._kernel_size, activation=tf.nn.relu, padding='same'
            )
            return tf.reduce_max(inputs, 1)


class RnnWordNNModel(BaseWordNNModel):
    _CLASSIFIER_NAME = 'rnn'
    _BATCH_FIRST = False

    def _add_encoder(self, inputs):
        with tf.variable_scope('encoder'):
            if self._is_train_model:
                inputs = tf.layers.dropout(inputs, rate=self._dropout_rate, training=self._is_training,
                                           noise_shape=tf.concat([[1], tf.shape(inputs)[1:]], axis=0))

            with tf.variable_scope('forward') as forward_scope:
                cell = LSTMBlockFusedCell(num_units=self._encoder_dim / 2)
                output_fw, (_, last_output_fw) = cell(
                    inputs, sequence_length=self._sequence_lengths if self._is_train_model else None,
                    dtype=tf.float32, scope=forward_scope
                )

            with tf.variable_scope('backward') as backward_scope:
                cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self._encoder_dim / 2))
                output_bw, (_, last_output_bw) = cell(
                    inputs, sequence_length=self._sequence_lengths if self._is_train_model else None,
                    dtype=tf.float32, scope=backward_scope
                )

            return tf.concat([last_output_fw, last_output_bw], axis=-1)
