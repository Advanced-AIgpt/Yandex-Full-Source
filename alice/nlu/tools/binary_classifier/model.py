# coding: utf-8

import json
import numpy as np
import os

from alice.nlu.tools.binary_classifier.metrics_counter import TrainingMetricsCounter
from alice.nlu.tools.binary_classifier.utils import is_ipython

if is_ipython():
    import tensorflow.compat.v1 as tf
    tf.disable_v2_behavior()
else:
    import tensorflow as tf

from tensorflow.python.layers import core as layers_core


class BinaryClassifier(object):
    _CHECKPOINT_NAME = 'model.ckpt'
    _MODEL_CONFIG_NAME = 'model_config.json'
    _INPUT_DESCRIPTION_NAME = 'input_description.json'

    def __init__(self, model_config, input_description, is_training):
        self._model_config = model_config
        self._input_description = input_description
        self._build_model(is_training)

    def _build_model(self, is_training):
        with tf.Graph().as_default() as graph:
            self._inputs = tf.placeholder(
                tf.float32,
                shape=[None, self._input_description['sentence']['vector_size']],
                name='inputs')

            outputs = self._inputs
            for hidden_layer_size in self._model_config['hidden_layer_sizes']:
                outputs = layers_core.Dense(hidden_layer_size, activation=tf.nn.relu)(outputs)

            self._logits = tf.squeeze(layers_core.Dense(1)(outputs), -1)
            self._probs = tf.sigmoid(self._logits, name='probs')

            if is_training:
                self._add_loss()
                self._add_optimizer()

            self._init_session(graph)
            self._init_saver(graph)

    def _add_loss(self):
        with tf.variable_scope('loss'):
            self._labels = tf.placeholder(tf.float32, shape=[None])

            losses = tf.nn.sigmoid_cross_entropy_with_logits(labels=self._labels, logits=self._logits)
            self._loss = tf.reduce_mean(losses)

    def _add_optimizer(self, lr=0.001, max_grad_norm=5.):
        with tf.variable_scope('optimizer'):
            optimizer = tf.train.AdamOptimizer(learning_rate=lr)

            grads, variables = zip(*optimizer.compute_gradients(self._loss))
            grads, global_norm = tf.clip_by_global_norm(grads, max_grad_norm)
            self._train_step = optimizer.apply_gradients(zip(grads, variables))

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
        saveable_vars = [variable for variable in saveable_vars if 'train' not in variable.name]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    def fit(self, epoch_count, train_data, valid_data=None):
        print('   Epoch  Train: ' + TrainingMetricsCounter.format_info_header() +
              ' Valid: ' + TrainingMetricsCounter.format_info_header())

        for epoch_index in range(epoch_count):
            print(' %3d /%3d' % (epoch_index + 1, epoch_count), end='')

            train_metrics = self._run_epoch(train_data, is_train_epoch=True)
            print('        ' + train_metrics.format_info_line(), end='')

            if valid_data is not None:
                val_metrics = self._run_epoch(valid_data, is_train_epoch=False)
                print('        ' + val_metrics.format_info_line(), end='')
            print()

    def _run_epoch(self, data, is_train_epoch):
        metrics = TrainingMetricsCounter()

        vectors = data['sentence_vectors']
        labels = data['labels']
        indices = np.arange(len(vectors))
        if is_train_epoch:
            np.random.shuffle(indices)
        fetches = [self._loss, self._probs] + ([self._train_step] if is_train_epoch else [])

        for batch_begin, batch_end in self._batch_intervals(len(indices)):
            batch_indices = indices[batch_begin:batch_end]
            batch_labels = labels[batch_indices]
            feed_dict = {
                self._inputs: vectors[batch_indices],
                self._labels: batch_labels
            }
            batch_loss, batch_probs = self._sess.run(fetches, feed_dict=feed_dict)[:2]
            batch_predictions = batch_probs > 0.5
            metrics.add_batch(target=batch_labels, pred=batch_predictions, loss=batch_loss)

        return metrics

    def _batch_intervals(self, dataset_size):
        batch_size = self._model_config['batch_size']
        for begin in range(0, dataset_size, batch_size):
            end = min(begin + batch_size, dataset_size)
            yield begin, end

    def predict(self, data):
        vectors = data['sentence_vectors']
        probs = []
        for batch_begin, batch_end in self._batch_intervals(len(vectors)):
            feed_dict = {
                self._inputs: vectors[batch_begin:batch_end]
            }
            probs.append(self._sess.run([self._probs], feed_dict=feed_dict)[0])
        return np.concatenate(probs, 0)

    def save(self, dir_path, save_to_protobuf_format=False):
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)

        with open(os.path.join(dir_path, self._MODEL_CONFIG_NAME), 'w') as f:
            json.dump(self._model_config, f, indent=2)
        with open(os.path.join(dir_path, self._INPUT_DESCRIPTION_NAME), 'w') as f:
            json.dump(self._input_description, f, indent=2)

        self._saver.save(self._sess, os.path.join(dir_path, self._CHECKPOINT_NAME))

        if save_to_protobuf_format:
            inference_model = self.restore(dir_path, is_training=False)
            self._save_protobuf_model(inference_model, dir_path)

    @staticmethod
    def _save_protobuf_model(pretrained_model, dir_path):
        graph_def = pretrained_model._sess.graph.as_graph_def()
        outputs = [pretrained_model._probs.op.name]
        frozen_graph_def = tf.graph_util.convert_variables_to_constants(pretrained_model._sess, graph_def, outputs)

        model_path = os.path.join(dir_path, 'model.pb')
        with open(model_path, 'wb') as f:
            f.write(frozen_graph_def.SerializeToString())

        model_description = {
            'input_node': pretrained_model._inputs.name,
            'output_node': pretrained_model._probs.name,
            'input_vector_size': pretrained_model._input_description['sentence']['vector_size'],
            'input_description': pretrained_model._input_description
        }

        with open(os.path.join(dir_path, 'model_description.json'), 'w') as f:
            json.dump(model_description, f, indent=2)

    @classmethod
    def restore(cls, dir_path, is_training=False):
        with open(os.path.join(dir_path, cls._MODEL_CONFIG_NAME)) as f:
            model_config = json.load(f)
        with open(os.path.join(dir_path, cls._INPUT_DESCRIPTION_NAME)) as f:
            input_description = json.load(f)

        model = cls(model_config, input_description, is_training=is_training)
        model._saver.restore(model._sess, os.path.join(dir_path, cls._CHECKPOINT_NAME))

        return model
