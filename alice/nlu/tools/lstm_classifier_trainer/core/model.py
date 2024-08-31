# coding: utf-8

import attr
import json
import logging
import os
import shutil
import tempfile
from enum import IntEnum

import numpy as np
import tensorflow as tf
from tensorflow.contrib.rnn import LSTMBlockFusedCell, TimeReversedFusedRNN
from tensorflow.python.layers import core as layers_core
from vins_models_tf import convert_model_to_memmapped, save_encoder_description

from alice.nlu.tools.lstm_classifier_trainer.core.metrics import create_metric
from alice.nlu.tools.lstm_classifier_trainer.core.data import ModelMode

logger = logging.getLogger(__name__)


class Reduction(object):
    '''Types of loss reduction.
    Contains the following values:
    * `MEAN`: Scalar sum of weighted losses divided by sum of weights.
    * `SUM_OVER_NONZERO_WEIGHTS`: Scalar sum of weighted losses divided by number of non-zero weights.
    '''

    MEAN = 'weighted_mean'
    SUM_BY_NONZERO_WEIGHTS = 'weighted_sum_by_nonzero_weights'

    @classmethod
    def all(cls):
        return (cls.MEAN, cls.SUM_BY_NONZERO_WEIGHTS)


@attr.s
class EmbedderConfig(object):
    word_emb_dim = attr.ib(default=300)
    dropout_rate = attr.ib(default=0.2)


@attr.s
class EncoderConfig(object):
    hidden_dim = attr.ib(default=128)
    layer_count = attr.ib(default=2)
    state_dropout_rate = attr.ib(default=0.2)
    output_dropout_rate = attr.ib(default=0.2)


@attr.s
class DecoderConfig(object):
    hidden_layer_dim = attr.ib(default=128)
    hidden_layer_count = attr.ib(default=0)
    class_count = attr.ib(default=2)


@attr.s
class TrainerConfig(object):
    mode = attr.ib(default=ModelMode.BINARY, converter=ModelMode)
    learning_rate = attr.ib(default=0.001)
    batch_size = attr.ib(default=128)
    max_grad_norm = attr.ib(default=5)
    epoch_count = attr.ib(default=20)
    use_weights = attr.ib(default=False)
    sqrt_weight_normalizer = attr.ib(default=False)
    loss_reduction = attr.ib(default=Reduction.SUM_BY_NONZERO_WEIGHTS, validator=attr.validators.in_(Reduction.all()))
    early_stopping_patience = attr.ib(default=3)
    shuffle_dataset = attr.ib(type=bool, default=False)
    metrics = attr.ib(default=[{'metric_name': 'accuracy'}])
    log_every = attr.ib(default=100)


@attr.s
class PredictConfig(object):
    batch_size = attr.ib(default=256)


@attr.s
class ModelConfig(object):
    embedder = attr.ib(default=EmbedderConfig())
    encoder = attr.ib(default=EncoderConfig())
    decoder = attr.ib(default=DecoderConfig())
    trainer = attr.ib(default=TrainerConfig())
    predict = attr.ib(default=PredictConfig())
    special_tokens = attr.ib(factory=list)
    train_special_token_embeddings = attr.ib(default=True)
    save_in_vins_models_tf_format = attr.ib(default=True)
    seed = attr.ib(default=None)

    @seed.validator
    def is_valid_seed(self, attribute, value):
        if value is not None and (0 > value or not isinstance(value, int)):
            raise ValueError("Seed has to be a natural number!")

    @classmethod
    def load(cls, json_config):
        embedder = EmbedderConfig(**json_config['embedder'])
        encoder = EncoderConfig(**json_config['encoder'])
        decoder = DecoderConfig(**json_config['decoder'])
        trainer = TrainerConfig(**json_config['trainer'])
        predict = PredictConfig(**json_config['predict'])
        special_tokens = json_config.get('special_tokens', [])
        seed = json_config.get('seed', None)

        return cls(
            embedder=embedder,
            encoder=encoder,
            decoder=decoder,
            trainer=trainer,
            predict=predict,
            special_tokens=special_tokens,
            seed=seed,
        )


class Model(object):
    class ModelState(IntEnum):
        TRAIN = 0
        VAL = 1

    def __init__(self, mode, config, embeddings_matrix=None):
        self._mode = mode
        self._config = config
        self._embeddings_matrix = embeddings_matrix

    def fit(self, train_data, valid_data=None):
        self._build_model()

        try:
            best_valid_loss, patience_epoch_count = None, 0
            for epoch in xrange(self._config.trainer.epoch_count):
                self._run_epoch(train_data, epoch, is_train=True)
                if valid_data is not None:
                    valid_loss = self._run_epoch(valid_data, epoch, is_train=False)
                    if best_valid_loss is None or valid_loss < best_valid_loss:
                        best_valid_loss, patience_epoch_count = valid_loss, 0
                    else:
                        patience_epoch_count += 1
                        if patience_epoch_count == self._config.trainer.early_stopping_patience:
                            logger.info('Run out of patience...')
                            return
        except KeyboardInterrupt:
            logger.info('Early stopping was triggered...')
            pass

    def _build_model(self):
        with tf.Graph().as_default() as graph:
            self._encoder_inputs = []
            self._sequence_lengths = tf.placeholder(tf.int32, shape=[None], name='sequence_lengths')

            self._features_dropout = tf.placeholder_with_default(1., shape=[], name='features_dropout')
            self._state_dropout = tf.placeholder_with_default(1., shape=[], name='state_dropout')
            self._rnn_out_dropout = tf.placeholder_with_default(1., shape=[], name='rnn_out_dropout')

            encoder_out = self._add_inputs()

            encoder_final_state = None
            for i in xrange(self._config.encoder.layer_count):
                encoder_out, encoder_final_state = self._add_rnn_encoder_layer(encoder_out, '_{}'.format(i + 1))
                encoder_final_state = tf.nn.dropout(encoder_final_state, self._rnn_out_dropout, seed=self._config.seed)

            self._add_decoder(encoder_final_state)

            if self._mode == self.ModelState.TRAIN:
                self._add_loss()
                self._add_optimizer(
                    lr=self._config.trainer.learning_rate,
                    max_grad_norm=self._config.trainer.max_grad_norm
                )

            self._init_session(graph)
            self._init_saver(graph)

    def _add_inputs(self):
        if self._mode != self.ModelState.TRAIN:
            with tf.variable_scope('inference'):
                self._token_ids_input = tf.placeholder(
                    tf.float32, shape=[None, None, self._config.embedder.word_emb_dim], name='token_embs'
                )
                rnn_inputs = self._token_ids_input
        else:
            with tf.variable_scope('train'):
                self._token_ids_input = tf.placeholder(tf.int32, shape=[None, None], name='token_ids')

                # following hack with placeholder is needed to load heavy embeds (> 2 GB)
                # https://stackoverflow.com/q/35394103
                self._embeddings_init = tf.placeholder(tf.float32, shape=self._embeddings_matrix.shape, name='embeddings_init')
                frozen_embeddings_var = tf.Variable(
                    self._embeddings_init, dtype=tf.float32,
                    trainable=False, name='embedding_matrix'
                )
                rnn_inputs = tf.nn.embedding_lookup(frozen_embeddings_var, self._token_ids_input)

                if self._config.special_tokens and self._config.train_special_token_embeddings:
                    self._trainable_embeddings_var = tf.get_variable(
                        name='trainable_embeddings',
                        shape=[len(self._config.special_tokens), self._embeddings_matrix.shape[-1]]
                    )
                    trainable_embeddings = tf.concat(
                        [tf.zeros((1, self._embeddings_matrix.shape[-1])), self._trainable_embeddings_var], 0
                    )  # first embed in self._embeddings_matrix is always OOV_TOKEN, make it non-trainable via tf.zeros
                    special_symbol_positions = tf.where(
                        self._token_ids_input < len(self._config.special_tokens) + 1,  # + 1 for OOV_TOKEN
                        self._token_ids_input,
                        tf.zeros_like(self._token_ids_input)
                    )
                    rnn_inputs += tf.nn.embedding_lookup(trainable_embeddings, special_symbol_positions)

        rnn_inputs = tf.transpose(rnn_inputs, perm=[1, 0, 2])
        return tf.nn.dropout(rnn_inputs, self._features_dropout, seed=self._config.seed)

    def _add_rnn_encoder_layer(self, inputs, name_suffix):
        with tf.variable_scope('bi-rnn' + name_suffix):
            inputs = tf.nn.dropout(
                inputs, self._state_dropout, noise_shape=tf.concat([[1], tf.shape(inputs)[1:]], axis=0), seed=self._config.seed
            )

            with tf.variable_scope('forward') as forward_scope:
                cell = LSTMBlockFusedCell(num_units=self._config.encoder.hidden_dim)
                output_fw, final_state_fw = cell(
                    inputs, sequence_length=self._sequence_lengths,
                    dtype=tf.float32, scope=forward_scope
                )

            with tf.variable_scope('backward') as forward_scope:
                cell = TimeReversedFusedRNN(LSTMBlockFusedCell(num_units=self._config.encoder.hidden_dim))
                output_bw, final_state_bw = cell(
                    inputs, sequence_length=self._sequence_lengths,
                    dtype=tf.float32, scope=forward_scope
                )

            output = tf.concat([output_fw, output_bw], axis=-1, name='output')
            final_state = tf.concat([final_state_fw.h, final_state_bw.h], axis=-1, name='final_state')
            return output, final_state

    def _add_decoder(self, inputs):
        class_prediction_layers = []
        for i in range(self._config.decoder.hidden_layer_count):
            class_prediction_layers.append(
                layers_core.Dense(self._config.decoder.hidden_layer_dim, activation=tf.nn.relu, name='class_output_layer_{}'.format(i))
            )

        class_prediction_layers.append(layers_core.Dense(self._config.decoder.class_count, name='class_output_layer'))

        for layer in class_prediction_layers:
            inputs = layer(inputs)

        self._logits = inputs

        if self._mode == self.ModelState.TRAIN:
            dtype = tf.float32 if self._config.trainer.mode in [ModelMode.BINARY, ModelMode.MULTILABEL] else tf.int32
            if self._config.trainer.mode == ModelMode.MULTILABEL:
                shape = [None, self._config.decoder.class_count]
            else:
                shape = [None]
            self._labels = tf.placeholder(dtype=dtype, shape=shape, name='class_labels')
            self._weights = tf.placeholder(dtype=tf.float32, shape=shape, name='class_weights')

        if self._config.decoder.class_count == 1 and self._mode == self.ModelState.TRAIN:
            self._logits = tf.squeeze(self._logits, -1)

        if self._config.trainer.mode in [ModelMode.BINARY, ModelMode.MULTILABEL]:
            self._probs = tf.nn.sigmoid(self._logits)
        else:
            self._probs = tf.nn.softmax(self._logits, axis=-1)

    def _add_loss(self):
        with tf.variable_scope('loss'):
            if self._config.trainer.mode == ModelMode.MULTICLASS:
                loss = tf.nn.sparse_softmax_cross_entropy_with_logits(
                    labels=self._labels,
                    logits=self._logits,
                    name='loss'
                )
                self._loss = tf.reduce_mean(loss)
            else:
                loss = tf.nn.sigmoid_cross_entropy_with_logits(
                    labels=self._labels,
                    logits=self._logits,
                    name='loss'
                )

            if not self._config.trainer.use_weights:
                self._loss = tf.reduce_mean(loss)
                return

            if self._config.trainer.loss_reduction == Reduction.MEAN:
                sum_weight = tf.reduce_sum(self._weights)
            elif self._config.trainer.loss_reduction == Reduction.SUM_BY_NONZERO_WEIGHTS:
                sum_weight = tf.reduce_sum(tf.cast(tf.not_equal(self._weights, 0.), tf.float32))
            else:
                assert False

            sum_loss = tf.reduce_sum(loss * self._weights)
            self._loss = tf.cond(tf.not_equal(sum_weight, 0.),
                                 lambda: sum_loss / sum_weight,
                                 lambda: 0.)

    def _add_optimizer(self, lr, max_grad_norm=-1.):
        with tf.variable_scope('optimizer'):
            optimizer = tf.train.AdamOptimizer(learning_rate=lr)

            if max_grad_norm > 0.:
                grads, variables = zip(*optimizer.compute_gradients(self._loss))
                grads, _ = tf.clip_by_global_norm(grads, max_grad_norm)
                self._optimizer = optimizer.apply_gradients(zip(grads, variables))
            else:
                self._optimizer = optimizer.minimize(self._loss)

    def _init_session(self, graph):
        config = tf.ConfigProto()
        config.gpu_options.allow_growth = True
        self._sess = tf.Session(config=config, graph=graph)

        feed_dict = None
        if self._mode == self.ModelState.TRAIN:
            feed_dict = {self._embeddings_init: self._embeddings_matrix}

        self._sess.run(tf.global_variables_initializer(), feed_dict=feed_dict)

    def _init_saver(self, graph):
        saveable_vars = (
            graph.get_collection(tf.GraphKeys.GLOBAL_VARIABLES)
            + graph.get_collection(tf.GraphKeys.SAVEABLE_OBJECTS)
        )
        saveable_vars = [variable for variable in saveable_vars if 'train' not in variable.name]
        self._saver = tf.train.Saver(var_list=saveable_vars)

    def get_inputs(self):
        return {
            'lengths': [self._sequence_lengths.name],
            'dense_seq': [self._token_ids_input.name]
        }

    def get_outputs(self):
        return self._probs.name

    @property
    def session(self):
        return self._sess

    def predict(self, batch_generator):
        rows, predictions = [], []
        for i, batch in enumerate(batch_generator):
            outputs = self._sess.run([self._probs], feed_dict=self._get_feed(batch, is_train=False))[0]

            rows.extend(batch['rows'])
            if self._config.decoder.class_count == 1:
                predictions.extend(np.squeeze(outputs, -1))
            else:
                predictions.extend(outputs)

            if i + 1 == len(batch_generator):
                return rows, predictions

    def _run_epoch(self, batch_generator, epoch, is_train):
        batches_count = len(batch_generator)

        logging_prefix = 'Train' if is_train else 'Valid'

        epoch_loss = 0.
        metrics = [create_metric(**metric_params) for metric_params in self._config.trainer.metrics]

        output_nodes = [self._loss, self._probs]
        if is_train:
            output_nodes.append(self._optimizer)

        for i, batch in enumerate(batch_generator):
            outputs = self._sess.run(output_nodes, feed_dict=self._get_feed(batch, is_train=is_train))

            epoch_loss += outputs[0]
            for metric in metrics:
                metric.update(outputs[1], batch['labels'])

            if self._config.trainer.log_every != -1 and i % self._config.trainer.log_every == 0:
                metrics_info = ', '.join(str(metric) for metric in metrics)
                logger.info('[{}] Batch = {} / {}. Loss = {:.4f}. {}'.format(
                    logging_prefix, i + 1, batches_count,
                    epoch_loss / (i + 1), metrics_info
                ))

            if i + 1 == batches_count:
                break

        metrics_info = ', '.join(str(metric) for metric in metrics)
        logger.info('[{}] Epoch = {} / {}. Loss = {:.4f}. {}'.format(
            logging_prefix, epoch + 1, self._config.trainer.epoch_count,
            epoch_loss / batches_count, metrics_info
        ))
        return epoch_loss / batches_count

    def _get_feed(self, batch, is_train):
        assert np.all(batch['token_ids'] < self._embeddings_matrix.shape[0])

        if self._mode == self.ModelState.TRAIN:
            if self._config.trainer.mode == ModelMode.MULTILABEL:
                assert np.all(0 <= np.array([x for y in batch['labels'] for x in y]))
                assert np.all(np.array([x for y in batch['labels'] for x in y]) <= self._config.decoder.class_count)
            else:
                assert np.all(0 <= batch['labels'])
                assert np.all(batch['labels'] <= self._config.decoder.class_count)

        if self._config.trainer.mode == ModelMode.MULTILABEL:
            # Converts to OHE
            if self._config.trainer.use_weights and self._mode == self.ModelState.TRAIN:
                converted_labels = []
                converted_weights = []
                for label_list, weight_list in zip(batch['labels'], batch['weights']):
                    labels = np.zeros(self._config.decoder.class_count)
                    weights = np.zeros(self._config.decoder.class_count)
                    weights_sum = np.sum(weight_list)
                    weights[:] = weights_sum
                    labels[label_list] = 1
                    weights[label_list] = weight_list
                    converted_labels.append(labels)
                    converted_weights.append(weights)

                batch['labels'] = np.array(converted_labels)
                batch['weights'] = np.array(converted_weights)
            else:
                converted_labels = []
                for label_list in batch['labels']:
                    labels = np.zeros(self._config.decoder.class_count)
                    labels[label_list] = 1
                    converted_labels.append(labels)

                batch['labels'] = np.array(converted_labels)

        feed = {
            self._sequence_lengths: batch['lengths']
        }

        if self._mode == self.ModelState.TRAIN:
            if self._config.trainer.use_weights:
                feed[self._weights] = batch['weights']
            feed[self._labels] = batch['labels']
            feed[self._token_ids_input] = batch['token_ids']
        else:
            feed[self._token_ids_input] = self._embeddings_matrix[batch['token_ids']]

        if is_train:
            feed[self._features_dropout] = 1. - self._config.embedder.dropout_rate
            feed[self._rnn_out_dropout] = 1. - self._config.encoder.output_dropout_rate
            feed[self._state_dropout] = 1. - self._config.encoder.state_dropout_rate

        return feed

    @property
    def trainable_embeddings(self):
        return self._sess.run(self._trainable_embeddings_var)

    def save(self, dir_path, base_name='model.ckpt'):
        '''
        Saves model's info into the dir_path dir
        '''
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)

        self._saver.save(self._sess, os.path.join(dir_path, base_name))

    def restore(self, dir_path, base_name='model.ckpt'):
        self._build_model()
        self._saver.restore(self._sess, os.path.join(dir_path, base_name))


class Trainer(object):
    _MODEL_DESCRIPTION_FILE_NAME = 'model_description.json'
    _MODEL_FILE_NAME = 'model.pb'

    def __init__(self, config, embeddings_matrix=None):
        self._config = config
        self._embeddings_matrix = embeddings_matrix
        assert self._config.embedder.word_emb_dim == embeddings_matrix.shape[1]

        for metric in self._config.trainer.metrics:
            metric['is_binary'] = self._config.trainer.mode in [ModelMode.BINARY, ModelMode.MULTILABEL]

        self._train_model = None

    def save(self, dir_path, min_conversion_size_bytes=100):
        '''
        Saves model's info into the dir_path dir
        '''
        if not os.path.isdir(dir_path):
            os.makedirs(dir_path)

        assert self._train_model, 'You should fit trainer before saving model'
        self._train_model.save(dir_path)
        self._save_trained_model(dir_path, min_conversion_size_bytes)

    def _save_trained_model(self, dir_path, min_conversion_size_bytes):
        if self._config.special_tokens and self._config.train_special_token_embeddings:
            self._save_trainable_embeddings(dir_path)

        inference_model = self._convert_model_for_inference()
        self._save_frozen_graph(dir_path, inference_model, min_conversion_size_bytes)

    def _convert_model_for_inference(self):
        temp_dir = None
        try:
            temp_dir = tempfile.mkdtemp('model_tmp')
            self._train_model.save(temp_dir)

            inference_model = Model(mode=Model.ModelState.VAL, config=self._config)
            inference_model.restore(temp_dir)
        finally:
            if temp_dir:
                shutil.rmtree(temp_dir)
        return inference_model

    def _save_trainable_embeddings(self, dir_path):
        trainable_embeddings = self._train_model.trainable_embeddings
        assert len(trainable_embeddings) == len(self._config.special_tokens)

        trainable_embeddings_json = {
            token: list(map(float, embedding))
            for token, embedding in zip(self._config.special_tokens, trainable_embeddings)
        }
        with open(os.path.join(dir_path, 'trainable_embeddings.json'), 'w') as f:
            json.dump(trainable_embeddings_json, f, indent=2)

    def _save_frozen_graph(self, dir_path, inference_model, min_conversion_size_bytes):
        graph_def = inference_model.session.graph.as_graph_def()

        inputs = inference_model.get_inputs()
        outputs = inference_model.get_outputs()
        graph_def = tf.graph_util.convert_variables_to_constants(
            inference_model.session, graph_def, [outputs.split(':')[0]]
        )

        model_path = os.path.join(dir_path, self._MODEL_FILE_NAME)
        with open(model_path, 'wb') as f:
            f.write(graph_def.SerializeToString())

        with open(os.path.join(dir_path, self._MODEL_DESCRIPTION_FILE_NAME), 'w') as f:
            model_description = {
                'inputs': {
                    'lengths': inputs['lengths'][0],
                    'dense_seq': inputs['dense_seq'][0]
                },
                'outputs': {
                    'class_probs': outputs
                }
            }
            json.dump(model_description, f, indent=2)

        if self._config.save_in_vins_models_tf_format:
            convert_model_to_memmapped(model_path, min_conversion_size_bytes)
            save_encoder_description(dir_path, (inputs, outputs))

    def fit(self, train_data, valid_data=None):
        self._train_model = Model(
            mode=Model.ModelState.TRAIN, config=self._config,
            embeddings_matrix=self._embeddings_matrix
        )
        self._train_model.fit(train_data, valid_data)

    def predict(self, data):
        return self._train_model.predict(data)
