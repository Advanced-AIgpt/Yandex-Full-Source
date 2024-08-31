import json
import numpy as np
import os
import logging
import tensorflow as tf
from tensorflow.python.layers import core as layers_core

from .metrics_counter import MetricsCounter


class BinaryClassifier(object):
    _CHECKPOINT_FOLDER = '/checkpoint/'
    _PROTO_FOLDER = '/proto/'
    _CHECKPOINT_NAME = 'model.ckpt'
    _TRAIN_CONFIG_NAME = 'train_config.json'

    def __init__(self, config, is_training):
        self._config = config
        self._build_model(config, is_training)
        self.threshold = config.threshold

    def _build_model(self, config, is_training):
        with tf.Graph().as_default() as graph:
            self._inputs = tf.placeholder(tf.float32, shape=[None, config.input_vector_size], name='inputs')

            outputs = self._inputs
            for hidden_layer_size in config.hidden_layer_sizes:
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

    def fit(self, train_data, valid_data=None, save_dir=None):
        best_val_loss = None
        for epoch_index in xrange(self._config.epoch_count):
            epoch_info = 'Train epoch {} / {}'.format(epoch_index + 1, self._config.epoch_count)
            self._run_epoch(
                vectors=train_data[0], labels=train_data[1], epoch_info=epoch_info, is_train_epoch=True
            )

            if valid_data is None:
                continue

            epoch_info = 'Valid epoch {} / {}'.format(epoch_index + 1, self._config.epoch_count)
            val_loss = self._run_epoch(
                vectors=valid_data[0], labels=valid_data[1], epoch_info=epoch_info, is_train_epoch=False
            )
            if save_dir is not None and (best_val_loss is None or val_loss < best_val_loss):
                self.save(save_dir)
                best_val_loss = val_loss

    def _run_epoch(self, vectors, labels, epoch_info, is_train_epoch):
        counter = MetricsCounter()

        if is_train_epoch:
            indexes = np.arange(vectors.shape[0])
            np.random.shuffle(indexes)
            vectors = vectors[indexes]
            labels = labels[indexes]

        for batch_loss, batch_probs, batch_labels in self._process_data(vectors, labels, is_train_epoch):
            batch_predictions = batch_probs > self.threshold
            counter.add_batch(batch_labels, batch_predictions, loss=batch_loss)

        logging.info('[%s] loss: %3.2f | precision: %3.2f | recall: %3.2f |  f1: %3.2f' % (
            epoch_info, counter.loss, counter.precision, counter.recall, counter.f1
        ))

        return counter.loss

    def _process_data(self, vectors, labels=None, is_train_epoch=False):
        train_step = [self._train_step] if is_train_epoch else []
        for batch_begin in xrange(0, len(vectors), self._config.batch_size):
            batch_end = min(batch_begin + self._config.batch_size, len(vectors))
            feed_dict = {
                self._inputs: vectors[batch_begin: batch_end]
            }
            if labels is None:
                yield self._sess.run([self._probs], feed_dict=feed_dict)[0]
            else:
                feed_dict[self._labels] = labels[batch_begin: batch_end]
                loss, probs = self._sess.run([self._loss, self._probs] + train_step, feed_dict=feed_dict)[:2]
                yield [loss, probs, labels[batch_begin: batch_end]]

    def predict(self, vectors):
        if self.threshold is None:
            raise ValueError('Needed to set threshold. For probas use predict_probas.')

        return self.predict_probas(vectors) >= self.threshold

    def predict_probas(self, vectors):
        probas = list(self._process_data(vectors))
        if len(probas) == 0:
            return np.array([])
            
        return np.concatenate(list(self._process_data(vectors)), 0)

    def save(self, save_to_protobuf_format=False):
        if not os.path.isdir(self._config.model_path):
            os.mkdir(self._config.model_path)
        
        if not os.path.isdir(self._config.model_path + self._CHECKPOINT_FOLDER):
            os.mkdir(self._config.model_path + self._CHECKPOINT_FOLDER)
        
        if not os.path.isdir(self._config.model_path + self._PROTO_FOLDER):
            os.mkdir(self._config.model_path + self._PROTO_FOLDER)
        
        with open(os.path.join(self._config.model_path + self._CHECKPOINT_FOLDER, self._TRAIN_CONFIG_NAME), 'w') as file:
            json.dump(self._config.get_train_config(), file, indent=2)

        self._saver.save(self._sess, os.path.join(self._config.model_path + self._CHECKPOINT_FOLDER, self._CHECKPOINT_NAME))
        
        inference_model = self.restore(self._config, is_training=False)
        self._save_protobuf_model(inference_model, self._config.model_path + self._PROTO_FOLDER)

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
            'input_vector_size': pretrained_model._config.input_vector_size,
            'threshold': pretrained_model.threshold
        }

        with open(os.path.join(dir_path, 'model_description.json'), 'w') as f:
            json.dump(model_description, f, indent=2)

    @classmethod
    def restore(cls, config, is_training=False):
        model_path = config.model_path + cls._CHECKPOINT_FOLDER
        with open(os.path.join(model_path, cls._TRAIN_CONFIG_NAME)) as file:
            config.set_train_config(json.load(file))
        
        model = cls(config, is_training=is_training)

        model._saver.restore(model._sess, os.path.join(model_path, cls._CHECKPOINT_NAME))

        return model
