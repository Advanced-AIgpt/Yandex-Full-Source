# coding: utf-8
from __future__ import unicode_literals

import os
import logging
import numbers
import math
from contextlib import contextmanager
from itertools import imap
from collections import Counter

import numpy as np
import scipy.sparse
import tensorflow as tf
from keras.layers import (
    Dense, Dropout, TimeDistributed, Bidirectional, Masking, Input,
    Conv1D, MaxPooling1D, Flatten, merge, GlobalMaxPooling1D,
    Activation, Embedding, add, concatenate
)
from keras.models import load_model, Sequential, Model
from keras.utils.np_utils import to_categorical
from keras.initializers import glorot_uniform
from keras import backend as keras_backend
from keras.regularizers import l2
from sklearn.base import BaseEstimator

from vins_core.utils.data import TarArchive
from vins_core.utils.config import get_bool_setting
from vins_core.utils.iter import first_of
from vins_core.nlu.features.base import SampleFeatures

from vins_models_tf import save_nn_classifier_model_description
from vins_models_tf import save_serialized_model_as_memmapped
from keras_utils import RNN, pad_sequences, define_type, SPARSE, DENSE, is_array_obj

logger = logging.getLogger(__name__)

_SAMPLE_FEATURES_EMBEDDINGS = 'sample_features'


@contextmanager
def no_gpu():
    """Restricts model created inside the context from gpu usage"""

    cuda_visible_devices = os.environ.get('CUDA_VISIBLE_DEVICES', None)

    os.environ['CUDA_VISIBLE_DEVICES'] = '-1'
    yield

    if cuda_visible_devices is None:
        del os.environ['CUDA_VISIBLE_DEVICES']
    else:
        os.environ['CUDA_VISIBLE_DEVICES'] = cuda_visible_devices


def _vectorize(sample_features):
    if is_array_obj(sample_features):
        return sample_features
    return map(lambda token_feature: first_of(token_feature.itervalues()), sample_features)


def _prepare_inputs(inputs, input_data_type, maxlen):
    if input_data_type == DENSE:
        input_dim = inputs[0].shape[-1]
        f = scipy.sparse.vstack(pad_sequences(x, maxlen, 'post') for x in inputs)
        return np.reshape(f.toarray(), (len(inputs), maxlen, input_dim)).astype(np.float32)
    elif input_data_type == SPARSE:
        return pad_sequences(inputs, maxlen, 'post')
    elif input_data_type == _SAMPLE_FEATURES_EMBEDDINGS:
        return pad_sequences(map(lambda x: x.dense_seq.values()[0], inputs), maxlen, 'post')


def _prepare_outputs(outputs, output_dim):
    return to_categorical(outputs, output_dim).astype(np.float32)


def _get_num_batches(num_samples, batch_size):
    return int(np.ceil(num_samples / float(batch_size)))


def _sample_features_generator(samples_features, labels, batch_size, input_data_type, maxlen, output_dim):
    num_samples = len(samples_features)
    indices = range(num_samples)
    num_batches = _get_num_batches(num_samples, batch_size)
    batches = [(i * batch_size, min(num_samples, (i + 1) * batch_size)) for i in xrange(num_batches)]
    while True:
        np.random.shuffle(indices)
        for batch_start, batch_end in batches:
            batch_x, batch_y = [], []
            for i in indices[batch_start:batch_end]:
                batch_x.append(samples_features[i])
                batch_y.append(labels[i])
            batch_x = _prepare_inputs(batch_x, input_data_type, maxlen)
            batch_y = _prepare_outputs(batch_y, output_dim)
            yield batch_x, batch_y


class GlobalMaxPooling1DMasked(GlobalMaxPooling1D):

    def __init__(self, **kwargs):
        super(GlobalMaxPooling1DMasked, self).__init__(**kwargs)
        self.supports_masking = True

    def call(self, x, mask=None):
        if mask is not None:
            m_tile = tf.tile(tf.expand_dims(mask, -1), tf.stack([1, 1, tf.shape(x)[-1]]))
            y = tf.where(m_tile, x, -np.inf * tf.ones_like(x))
            return super(GlobalMaxPooling1DMasked, self).call(y)
        else:
            return super(GlobalMaxPooling1DMasked, self).call(x)

    def get_output_shape_for(self, input_shape):
        return input_shape[0], input_shape[-1]

    def compute_mask(self, input, input_mask=None):
        return None


class NNClassifierModel(BaseEstimator):
    _MODEL_FILE_NAME = 'model.mmap'

    def __init__(self, batch_size=128, nb_epoch=20, optimizer='adam', maxlen=None,
                 input_projection_dim=None, encoder_dim=128,
                 dropout=0.25, verbose=True, random_seed=42, class_balancing=None, embedding_dim=None, **kwargs):
        """
        :param batch_size: batch size both for training and inference
        :param nb_epoch: number of training epochs
        :param optimizer: optimizer (see https://keras.io/optimizers/)
        :param maxlen: the number of tokens to be padded or truncated. If None, is taken from the max length during fit
        :param input_projection_dim: first layer size (linear projection)
        :param encoder_dim: second layer size (recurrent encoder, convolution filters, etc.)
        :param dropout: dropout probability
        :param verbose: print info
        :param random_seed: random seed
        :param class_balancing: the method to compute class weight on fitting: None, freq, median_freq, log_freq
        :param kwargs: for сompatibility
        """
        self.batch_size = batch_size
        self.nb_epoch = nb_epoch
        self.optimizer = optimizer
        self.maxlen = maxlen
        self.input_projection_dim = input_projection_dim
        self.encoder_dim = encoder_dim
        self.dropout = dropout
        self.verbose = verbose
        self.random_seed = random_seed
        self.class_balancing = class_balancing
        self.embedding_dim = embedding_dim

        self._model = None
        self._input_dim = None
        self._output_dim = None
        self._input_data_type = None
        self._filepath_base_name = None
        self._class_weights = None

    @property
    def input_dim(self):
        return self._input_dim

    @property
    def output_dim(self):
        return self._output_dim

    @staticmethod
    def _validate(inputs, outputs):
        if len(inputs) == 0:
            raise ValueError('Empty inputs')
        assert all(isinstance(item, numbers.Integral) for item in outputs)
        input_data_type = define_type(inputs)
        if input_data_type != DENSE and input_data_type != SPARSE:
            # SampleFeatures accepted only if they contain single embedding vector
            for item in inputs:
                assert isinstance(item, SampleFeatures)
                assert item.dense_seq
                if item.dense or item.sparse or item.sparse_seq or len(item.dense_seq) > 1:
                    raise NotImplementedError(
                        'Using plain SampleFeatures as input only allowed with single embedding feature. '
                        'Otherwise consider to use vectorizer=true model option'
                    )

    def _initialize(self, inputs, outputs):
        self._input_data_type = define_type(inputs)
        if self._input_data_type == SPARSE:
            # each sample represented as list of token indices
            self._input_dim = max(imap(max, inputs)) + 1
            if not self.maxlen:
                self.maxlen = max(imap(len, inputs))
        elif self._input_data_type == DENSE:
            # each sample represented as matrix
            self._input_dim = inputs[0].shape[-1]
            if not self.maxlen:
                self.maxlen = max(imap(lambda feature: feature.shape[0], inputs))
        else:
            self._input_data_type = _SAMPLE_FEATURES_EMBEDDINGS
            # each sample represented as SampleFeatures instance
            self._input_dim = inputs[0].dense_seq.values()[0].shape[-1]
            if not self.maxlen:
                self.maxlen = max(imap(len, inputs))
        self._output_dim = max(outputs) + 1
        if keras_backend.backend() == 'tensorflow':
            self._graph = tf.get_default_graph()

    def _create(self):
        raise NotImplementedError()

    def _class_weight(self, outputs):
        class_counts = Counter(outputs)
        if not self.class_balancing:
            return dict.fromkeys(class_counts.keys(), 1)
        if self.class_balancing == 'freq':
            return {class_idx: 1. / count for class_idx, count in class_counts.iteritems()}
        elif self.class_balancing == 'median_freq':
            median_count = float(np.median(class_counts.values()))
            return {class_idx: median_count / count for class_idx, count in class_counts.iteritems()}
        elif self.class_balancing == 'log_freq':
            total_count = float(len(outputs))
            return {class_idx: max(1, math.log(total_count / count)) for class_idx, count in class_counts.iteritems()}

    def _create_input_net(self, masking):
        if self._input_data_type == DENSE or self._input_data_type == _SAMPLE_FEATURES_EMBEDDINGS:
            input = Input(shape=(self.maxlen, self._input_dim), dtype='float32')
            if masking:
                net = Masking(input_shape=(self.maxlen, self._input_dim))(input)
            else:
                net = input
            if self.input_projection_dim:
                net = TimeDistributed(Dense(
                    self.input_projection_dim, kernel_initializer=glorot_uniform(self.random_seed), activation='relu')
                )(net)
        else:
            if not self.input_projection_dim:
                raise ValueError('With categorical input input_projection_dim should be positive integer')
            input = Input(shape=(self.maxlen,), dtype='int32')
            net = Embedding(self._input_dim, self.input_projection_dim, mask_zero=masking)(input)
        return input, net

    def get_tf_input_tensors(self):
        learning_phase_tensors = []
        for op in self._graph.get_operations():
            if op.name.startswith('dropout') and op.name.endswith('keras_learning_phase') and len(op.values()) == 1:
                learning_phase_tensors.append(op.values()[0])
        input_tensors = {
            'inputs': self._model.inputs[0],
            'targets': self._model.targets[0],
            'weights': self._model.sample_weights[0],
            'learning_phase': learning_phase_tensors
        }
        return input_tensors

    def get_tf_input_feed(self, preprocessed_features, preprocessed_labels, input_tensors):
        features = _prepare_inputs(preprocessed_features, self._input_data_type, self.maxlen)
        labels = _prepare_outputs(preprocessed_labels, self.output_dim)
        weights = [self._class_weights[class_idx] for class_idx in np.argmax(labels, axis=1)]
        input_feed = {
            input_tensors['inputs']: features,
            input_tensors['targets']: labels,
            input_tensors['weights']: weights
        }
        for learning_phase_tensor in input_tensors['learning_phase']:
            input_feed[learning_phase_tensor] = False
        return input_feed

    def fit(self, inputs, outputs):
        self._validate(inputs, outputs)
        logger.info('Initializing the model "%s"', self.__class__.__name__)
        self._initialize(inputs, outputs)
        self._model = self._create()
        self._model.compile(
            optimizer=self.optimizer,
            loss='categorical_crossentropy'
        )
        logger.info('Start fitting...')
        self._class_weights = self._class_weight(outputs)
        self._model.fit_generator(
            generator=_sample_features_generator(
                inputs, outputs, self.batch_size, self._input_data_type, self.maxlen, self._output_dim),
            steps_per_epoch=_get_num_batches(len(inputs), self.batch_size),
            epochs=self.nb_epoch,
            verbose=self.verbose,
            class_weight=self._class_weights
        )
        if self.verbose:
            self._model.summary()
        return self

    def _predict_proba(self, inputs):
        output = []
        nsplits = len(inputs) / self.batch_size
        batch_splits = np.array_split(inputs, nsplits) if nsplits > 1 else [inputs]
        for x in batch_splits:
            output.append(self._model.predict_on_batch(
                _prepare_inputs(x, self._input_data_type, self.maxlen)
            ))
        return np.vstack(output)

    def predict_proba(self, inputs):
        if not self._model and self._filepath_base_name:
            filepath, base, name = self._filepath_base_name
            with TarArchive(filepath) as archive:
                if base:
                    with archive.nested(base) as arch:
                        self._load(arch, name)
                else:
                    self._load(archive, name)
            logger.info('Model "%s" has been loaded %s '
                        'from archive=%s, base=%s, name=%s',
                        self.__class__.__name__, 'on inference', filepath, base, name)
        elif not self._model and not self._filepath_base_name:
            raise ValueError('Lazy loading on inference without archive specified: '
                             'Load archive by running %s.load() before', self.__class__.__name__)
        if keras_backend.backend() == 'tensorflow':
            with self._graph.as_default():
                return self._predict_proba(inputs)
        return self._predict_proba(inputs)

    def predict(self, inputs):
        return np.argmax(self.predict_proba(inputs), axis=-1)

    def _save_model_features(self, model_dir_name):
        input_tensors = self.get_tf_input_tensors()
        model_features = (
            self.batch_size,
            self.maxlen,
            input_tensors['inputs'].name,
            self._model.outputs[0].name,
            [t.name for t in input_tensors['learning_phase']]
        )
        save_nn_classifier_model_description(model_dir_name, model_features)

    def _save_frozen_graph(self, model_dir_name, min_conversion_size_bytes):
        sess = keras_backend.get_session()
        graph_def = sess.graph.as_graph_def()
        graph_def = tf.graph_util.convert_variables_to_constants(
            sess,
            graph_def,
            [self._model.outputs[0].op.name]
        )
        model_path = os.path.join(model_dir_name, self._MODEL_FILE_NAME)
        serialized_graph = graph_def.SerializeToString()
        save_serialized_model_as_memmapped(
            model_path,
            serialized_graph,
            min_conversion_size_bytes
        )

    def save_to_dir(self, model_dir_name, min_conversion_size_bytes):
        if not os.path.isdir(model_dir_name):
            os.makedirs(model_dir_name)
        self._save_model_features(model_dir_name)
        self._save_frozen_graph(model_dir_name, min_conversion_size_bytes)

    def save(self, archive, name, min_conversion_size_bytes=100):
        # This block loads model parameters if it wasn't loaded before
        # this may happens if VINS_LOAD_TF_ON_CALL=1 and predict_proba method is never called
        # TODO: join this block with predict_proba
        if not self._model and self._filepath_base_name:
            filepath, base, name = self._filepath_base_name
            with TarArchive(filepath) as load_archive:
                if base:
                    with load_archive.nested(base) as arch:
                        self._load(arch, name)
                else:
                    self._load(archive, name)
            logger.info('Model "%s" has been loaded %s '
                        'from archive=%s, base=%s, name=%s',
                        self.__class__.__name__, 'on inference', filepath, base, name)
        if self._model:
            tmp = archive.get_tmp_file()
            self._model.save(tmp)
            archive.add(name + '.h5', tmp)
        else:
            logger.warning('File %s was not created because model is empty', name)

    def _load(self, archive, name):
        with no_gpu():
            tmp_dir = archive.get_tmp_dir()
            tmp_file = os.path.join(tmp_dir, archive.base, name + '.h5')
            archive.save_by_name(name + '.h5', tmp_dir)
            if keras_backend.backend() == 'tensorflow':
                self._graph = tf.get_default_graph()
                with self._graph.as_default():
                    self._model = load_model(
                        tmp_file, custom_objects={'GlobalMaxPooling1DMasked': GlobalMaxPooling1DMasked})
                    # this call is necessary to avoid data races with TF ops initialization
                    # when inference is running in different threads
                    self._model._make_predict_function()
            else:
                self._model = load_model(
                    tmp_file, custom_objects={'GlobalMaxPooling1DMasked': GlobalMaxPooling1DMasked})

    def load(self, archive, name):
        if get_bool_setting('LOAD_TF_ON_CALL'):
            self._filepath_base_name = (archive.path, archive.base.rstrip('/'), name)
        else:
            self._filepath_base_name = None
            self._load(archive, name)
            logger.info('Model "%s" has been loaded %s '
                        'from archive=%s, base=%s, name=%s',
                        self.__class__.__name__, 'on load', archive.path, archive.base, name)

    def __getstate__(self):
        state = super(NNClassifierModel, self).__getstate__()
        state['_model'] = None
        state['_graph'] = None
        state['_filepath_base_name'] = None
        return state

    def set_params(self, **params):
        super(NNClassifierModel, self).set_params(**params)
        self._model = None


class RNNClassifierModel(NNClassifierModel):
    def __init__(self, batch_size=128, nb_epoch=20, optimizer='adam', maxlen=16,
                 input_projection_dim=None, encoder_dim=128,
                 encoder_type='lstm', encoder_bidirectional=True, class_balancing='log_freq',
                 output_projection_dim=128, dropout=0.5, max_pooling=True, masking=True, unroll=False, **kwargs):
        """
        :param batch_size: batch size both for training and inference
        :param nb_epoch: number of training epochs
        :param optimizer: optimizer (see https://keras.io/optimizers/)
        :param maxlen: the number of tokens to be padded or truncated. If None, is taken from the max length during fit
        :param input_projection_dim: first layer size (linear projection)
        :param encoder_dim: hidden layers size for specified recurrent encoder
        :param encoder_type: type of recurrent encoder
        :param encoder_bidirectional: if True, uses bidirectional encoder
        :param output_projection_dim: if not None, specifies the size of fully-connected layer after RNN
        :param dropout: dropout probability
        :param verbose: print info
        :param random_seed: random seed
        :param class_balancing: the method to compute class weight on fitting: None, freq, median_freq, log_freq
        :param kwargs: for сompatibility
        """
        super(RNNClassifierModel, self).__init__(**kwargs)
        self.batch_size = batch_size
        self.nb_epoch = nb_epoch
        self.optimizer = optimizer
        self.maxlen = maxlen
        self.input_projection_dim = input_projection_dim
        self.encoder_dim = encoder_dim
        self.dropout = dropout
        self.encoder_type = encoder_type
        self.encoder_bidirectional = encoder_bidirectional
        self.output_projection_dim = output_projection_dim
        self.class_balancing = class_balancing
        self.max_pooling = max_pooling
        self.masking = masking
        self.unroll = unroll

    def _create(self):
        input, net = self._create_input_net(masking=self.masking)
        if self.encoder_bidirectional:
            logger.warning('Unable to use custom initializer for Bidirectional wrapper: Keras bug')
            net = Bidirectional(RNN(self.encoder_type)(self.encoder_dim, return_sequences=self.max_pooling,
                                                       unroll=self.unroll))(net)
        else:
            net = RNN(self.encoder_type)(self.encoder_dim, kernel_initializer=glorot_uniform(self.random_seed),
                                         return_sequences=self.max_pooling, unroll=self.unroll)(net)
        if self.max_pooling:
            net = GlobalMaxPooling1DMasked()(net)
        net = Dropout(self.dropout)(net)
        if self.output_projection_dim:
            net = Dense(self.output_projection_dim, activation='relu')(net)
        output = Dense(
            self._output_dim, activation='softmax', kernel_initializer=glorot_uniform(self.random_seed),
            kernel_regularizer=l2()
        )(net)
        return Model(inputs=input, outputs=output)


class CNNClassifierModel(NNClassifierModel):

    def __init__(self, batch_size=128, nb_epoch=20, optimizer='adam', maxlen=None,
                 input_projection_dim=None, encoder_dim=128, ngram_range=(1, 3),
                 dropout=0.25, fc_layers=(), class_balancing=None, **kwargs):
        """
        :param batch_size: batch size both for training and inference
        :param nb_epoch: number of training epochs
        :param optimizer: optimizer (see https://keras.io/optimizers/)
        :param maxlen: the number of tokens to be padded or truncated. If None, is taken from the max length during fit
        :param input_projection_dim: first layer size (linear projection)
        :param encoder_dim: conv layer size
        :param ngram_range: conv layers filter size range (excluding upper bound)
        :param dropout: dropout probability
        :param fc_layers: list of top-level fully connected tanh layers sizes
        :param class_balancing: the method to compute class weight on fitting: None, freq, median_freq, log_freq
        :param verbose: print info
        :param random_seed: random seed
        :param kwargs: for сompatibility
        """
        super(CNNClassifierModel, self).__init__(**kwargs)
        self.batch_size = batch_size
        self.nb_epoch = nb_epoch
        self.optimizer = optimizer
        self.maxlen = maxlen
        self.input_projection_dim = input_projection_dim
        self.encoder_dim = encoder_dim
        self.dropout = dropout
        self.ngram_range = ngram_range
        self.class_balancing = class_balancing
        self.fc_layers = fc_layers

    def _create_conv_layers(self, input_layer):
        convnets = []
        for ngram in range(*self.ngram_range):
            c = Conv1D(
                filters=self.encoder_dim,
                kernel_size=ngram,
                activation='relu',
                kernel_initializer=glorot_uniform(self.random_seed)
            )(input_layer)
            c = MaxPooling1D(pool_size=self.maxlen - ngram + 1)(c)
            c = Flatten()(c)
            convnets.append(c)
        return concatenate(convnets) if len(convnets) > 1 else convnets[0]

    def _create(self):
        input, net = self._create_input_net(masking=False)
        proj = Dropout(self.dropout)(net)
        output = self._create_conv_layers(proj)
        for fc_layer in self.fc_layers:
            output = Dense(fc_layer, activation='relu', kernel_initializer=glorot_uniform(self.random_seed))(output)
            output = Dropout(self.dropout)(output)
        output = Dense(
            self._output_dim, activation='softmax', kernel_initializer=glorot_uniform(self.random_seed)
        )(output)
        return Model(inputs=input, outputs=output)


class ResidualCNNClassifierModel(CNNClassifierModel):

    def __init__(self, batch_size=128, nb_epoch=20, optimizer='adam', maxlen=None,
                 input_projection_dim=None, encoder_dim=128, ngram_range=(1, 3),
                 dropout=0.25, class_balancing=None, **kwargs):
        """
        :param batch_size: batch size both for training and inference
        :param nb_epoch: number of training epochs
        :param optimizer: optimizer (see https://keras.io/optimizers/)
        :param maxlen: the number of tokens to be padded or truncated. If None, is taken from the max length during fit
        :param input_projection_dim: first layer size (linear projection)
        :param encoder_dim: conv layer size
        :param ngram_range: conv layers filter size range (excluding upper bound)        :
        :param dropout: dropout probability
        :param verbose: print info
        :param random_seed: random seed
        :param class_balancing: the method to compute class weight on fitting: None, freq, median_freq, log_freq
        :param kwargs: for сompatibility
        """
        super(ResidualCNNClassifierModel, self).__init__(**kwargs)
        self.batch_size = batch_size
        self.nb_epoch = nb_epoch
        self.optimizer = optimizer
        self.maxlen = maxlen
        self.input_projection_dim = input_projection_dim
        self.encoder_dim = encoder_dim
        self.dropout = dropout
        self.ngram_range = ngram_range
        self.class_balancing = class_balancing

    def _create(self):
        input, net = self._create_input_net(masking=False)
        proj = Dropout(self.dropout)(net)
        embedded_input = self._create_conv_layers(proj)
        shortcut = Sequential()
        shortcut.add(GlobalMaxPooling1D(input_shape=(self.maxlen, self._input_dim)))
        proj_dim = len(xrange(*self.ngram_range)) * self.encoder_dim
        shortcut.add(Dense(proj_dim, kernel_initializer=glorot_uniform(self.random_seed)))
        residual = Activation('relu')(merge((embedded_input, shortcut(input)), mode='sum'))
        output = Dense(
            self._output_dim, activation='softmax', kernel_initializer=glorot_uniform(self.random_seed)
        )(residual)
        return Model(inputs=input, outputs=output)


class ResidualRNNClassifierModel(RNNClassifierModel):

    def __init__(self, batch_size=128, nb_epoch=20, optimizer='adam', maxlen=None,
                 input_projection_dim=None, encoder_dim=128,
                 encoder_type='lstm', encoder_bidirectional=False,
                 dropout=0.25, output_projection_dim=None,
                 class_balancing=None, **kwargs):
        """
        :param batch_size: batch size both for training and inference
        :param nb_epoch: number of training epochs
        :param optimizer: optimizer (see https://keras.io/optimizers/)
        :param maxlen: the number of tokens to be padded or truncated. If None, is taken from the max length during fit
        :param input_projection_dim: first layer size (linear projection)
        :param encoder_dim: hidden layers size for specified recurrent encoder
        :param encoder_type: type of recurrent encoder
        :param encoder_bidirectional: if True, uses bidirectional encoder
        :param dropout: dropout probability
        :param verbose: print info
        :param random_seed: random seed
        :param class_balancing: the method to compute class weight on fitting: None, freq, median_freq, log_freq
        :param kwargs: for сompatibility
        """
        super(ResidualRNNClassifierModel, self).__init__(**kwargs)
        self.batch_size = batch_size
        self.nb_epoch = nb_epoch
        self.optimizer = optimizer
        self.maxlen = maxlen
        self.input_projection_dim = input_projection_dim
        self.encoder_dim = encoder_dim
        self.dropout = dropout
        self.encoder_type = encoder_type
        self.encoder_bidirectional = encoder_bidirectional
        self.output_projection_dim = output_projection_dim
        self.class_balancing = class_balancing

    def _create(self):

        input, net = self._create_input_net(masking=True)
        rnn_model = Dropout(self.dropout)(net)
        if self.encoder_bidirectional:
            logger.warning('Unable to use custom initializer for Bidirectional wrapper: Keras bug')
            rnn_model = Bidirectional(RNN(self.encoder_type)(self.encoder_dim))(rnn_model)
        else:
            rnn_model = RNN(self.encoder_type)(
                self.encoder_dim, kernel_initializer=glorot_uniform(self.random_seed)
            )(rnn_model)

        shortcut = Sequential()
        shortcut.add(GlobalMaxPooling1D(input_shape=(self.maxlen, self._input_dim)))
        proj_dim = 2 * self.encoder_dim if self.encoder_bidirectional else self.encoder_dim
        shortcut.add(Dense(proj_dim, kernel_initializer=glorot_uniform(self.random_seed)))

        residual = Activation('relu')(add([rnn_model, shortcut(input)]))
        if self.output_projection_dim:
            residual = Dense(self.output_projection_dim, activation='tanh')(residual)
        output = Dense(
            self._output_dim, activation='softmax', kernel_initializer=glorot_uniform(self.random_seed)
        )(residual)
        return Model(inputs=input, outputs=output)
