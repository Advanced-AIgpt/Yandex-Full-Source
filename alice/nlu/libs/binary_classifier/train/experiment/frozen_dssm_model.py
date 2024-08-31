# coding: utf-8

import argparse
import attr
import json
import logging
import numpy as np
import os
import shutil
import tempfile
import tensorflow as tf

from tensorflow.python.layers import core as layers_core

from dataset_utils import DatasetConfig, load_dataset, prepare_dssm_data, download_dataset
from metric_utils import compute_metrics

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class BinaryClassifierTrainConfig(object):
    input_vector_size = attr.ib()
    hidden_layer_sizes = attr.ib()
    batch_size = attr.ib()
    epoch_count = attr.ib()


def _build_f1_counter():
    counts = [0., 0., 0.]

    def _counter(y_true, y_pred):
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
    return _counter


class BinaryClassifier(object):
    _CHECKPOINT_NAME = 'model.ckpt'
    _TRAIN_CONFIG_NAME = 'train_config.json'

    def __init__(self, config, is_training):
        self._config = config
        self._build_model(config, is_training)

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
        saveable_vars = [variable for variable in saveable_vars if not 'train' in variable.name]
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
        total_loss, batches_count = 0., 0
        f1_counter = _build_f1_counter()

        for batch_loss, batch_probs, batch_labels in self._process_data(vectors, labels, is_train_epoch):
            batch_predictions = batch_probs > 0.5
            f1_score = f1_counter(y_true=batch_labels, y_pred=batch_predictions)

            total_loss += batch_loss
            batches_count += 1

        logger.info('[{}] Loss = {:.3f}, F1 = ({:.2%} / {:.2%} / {:.2%})'.format(
            epoch_info, total_loss / batches_count, *f1_score
        ))

        return total_loss / batches_count

    def _process_data(self, vectors, labels=None, is_train_epoch=False):
        indices = np.arange(len(vectors))
        if is_train_epoch:
            np.random.shuffle(indices)

        train_step = [self._train_step] if is_train_epoch else []
        for batch_begin in xrange(0, len(indices), self._config.batch_size):
            batch_end = batch_begin + min(self._config.batch_size, len(indices))
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
        return np.concatenate(list(self._process_data(vectors)), 0)

    def save(self, dir_path, save_to_protobuf_format=False):
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)

        with open(os.path.join(dir_path, self._TRAIN_CONFIG_NAME), 'w') as f:
            json.dump(attr.asdict(self._config), f, indent=2)

        if not save_to_protobuf_format:
            self._saver.save(self._sess, os.path.join(dir_path, self._CHECKPOINT_NAME))
        else:
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
            'input_vector_size': pretrained_model._config.input_vector_size
        }

        with open(os.path.join(dir_path, 'model_description.json'), 'w') as f:
            json.dump(model_description, f, indent=2)

    @classmethod
    def restore(cls, dir_path, is_training=False):
        with open(os.path.join(dir_path, cls._TRAIN_CONFIG_NAME)) as f:
            config = BinaryClassifierTrainConfig(**json.load(f))

        model = cls(config, is_training=is_training)
        model._saver.restore(model._sess, os.path.join(dir_path, cls._CHECKPOINT_NAME))

        return model


def _train(train_dataset, valid_dataset, embedding_type, input_vector_size=300):
    vectorizer_params = {
        'source': 'dssm',
        'type': embedding_type,
    }

    train_vectors, train_labels, valid_vectors, valid_labels, _ = prepare_dssm_data(
        train_dataset, valid_dataset, vectorizer_params
    )

    config = BinaryClassifierTrainConfig(
        input_vector_size=input_vector_size,
        hidden_layer_sizes=[300, 300],
        batch_size=512,
        epoch_count=10
    )

    model = BinaryClassifier(config=config, is_training=True)

    output_dir = 'models/{}_frozen_dssm'.format(embedding_type)
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)

    model.fit((train_vectors, train_labels), (valid_vectors, valid_labels), output_dir)

    train_predictions = model.predict(train_vectors)
    valid_predictions = model.predict(valid_vectors)

    compute_metrics(train_predictions, train_labels, output_dir, 'train')
    compute_metrics(valid_predictions, valid_labels, output_dir, 'valid')

    model.save(output_dir, save_to_protobuf_format=True)

    with open(os.path.join(output_dir, 'config.json'), 'w') as f:
        model_config = {
            'vectorizer_params': {
                'source': 'dssm',
                'type': embedding_type,
                'input_vector_size': input_vector_size
            }
        }
        json.dump(model_config, f, indent=2)


def _run_experiments(dataset_config_path):
    with open(dataset_config_path) as f:
        dataset_config = DatasetConfig(**json.load(f))

    train_dataset, valid_dataset = load_dataset(dataset_config)

    embedding_types = [key for key in train_dataset[0] if key.endswith('embedding')]
    for embedding_type in ['boltalka_small_context_embedding']:
        _train(train_dataset, valid_dataset, embedding_type=embedding_type)


def _run_evaluation(dataset_config_path, model_dir, eval_table_path):
    with open(dataset_config_path) as f:
        dataset_config = DatasetConfig(**json.load(f))

    with open(os.path.join(model_dir, 'config.json')) as f:
        model_config = json.load(f)

    dataset = download_dataset(eval_table_path)

    test_vectors = np.array([sample[model_config['vectorizer_params']['type']] for sample in dataset])
    test_labels = np.array([int(sample['intent'] == dataset_config.intent) for sample in dataset])

    model = BinaryClassifier.restore(model_dir)

    test_predictions = model.predict(test_vectors)

    output_prefix = eval_table_path.split('/')[-1]
    compute_metrics(test_predictions, test_labels, model_dir, output_prefix)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--mode', choices=['experiments', 'evaluation'], required=True)
    parser.add_argument('--dataset-config-path', required=True)
    parser.add_argument('--model-dir')
    parser.add_argument('--eval-table-path')
    args = parser.parse_args()

    if args.mode == 'experiments':
        _run_experiments(args.dataset_config_path)
    elif args.mode == 'evaluation':
        _run_evaluation(args.dataset_config_path, args.model_dir, args.eval_table_path)
    else:
        assert False, 'Unknown mode: {}'.format(args.mode)


if __name__ == "__main__":
    main()
