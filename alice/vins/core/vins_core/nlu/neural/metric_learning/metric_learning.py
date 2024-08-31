# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import numpy as np
import os
import json

from collections import OrderedDict
from operator import itemgetter
from enum import Enum

from vins_core.nlu.features.post_processor.base import BaseFeaturesPostProcessor
from vins_core.utils.config import get_bool_setting
from vins_core.utils.data import vins_temp_dir, get_resource_full_path
from vins_core.nlu.features.post_processor.sparse_indexer import SparseIndexerFeaturesPostProcessor
from vins_core.nlu.features.post_processor.dense_features_guard import DenseFeaturesGuard
from vins_core.nlu.neural.metric_learning.sample_features_preprocessor import \
    SampleFeaturesPreprocessor, SampleFeaturesTrainPreprocessor
from vins_core.nlu.neural.metric_learning.model import MetricLearningModel
from vins_core.nlu.neural.metric_learning.config import MetricLearningConfig
from vins_core.nlu.neural.metric_learning.trainer import Trainer, Applier
from vins_core.nlu.neural.metric_learning.callbacks.batch_schedule import BatchSchedule


logger = logging.getLogger(__name__)


class TrainMode(Enum):
    NO_METRIC_LEARNING = 'NO_METRIC_LEARNING'
    METRIC_LEARNING_FROM_SCRATCH = 'METRIC_LEARNING_FROM_SCRATCH'


class MetricLearningFeaturesPostProcessor(BaseFeaturesPostProcessor):
    def __init__(self, model_dir=None, batch_samples_per_class=80, sparse_seq_dim=50, sparse_dim=50,
                 train=True, train_split=0.95, num_epochs=10, encoder_num_units=128, encoder_pooling='last',
                 encoder_num_layers=3, restore_weights_mode='last', oov='ignore', num_classes_in_batch=None,
                 loss=None, class_sampling='freq', lr=1e-3, lr_decay=None, lr_decay_steps=1000, model_name='baseline',
                 logdir=None, l2norm=True, checkpoint_freq_in_batches=50, exclude_intents_from_validation=None,
                 encoder_output_dense_layers=None, num_updates=None, batch_schedule=None, valid_num_classes=None,
                 valid_samples_per_class=None, features_extractor_info=None, finetune_embeddings=False,
                 save_weights_mode='best', embeddings_map_key=None, save_weights_start=0, batch_generator_njobs=1,
                 batch_generator_queue_maxsize=1, transform_batch_size=1000, **kwargs):
        """
        :param model_dir: base path to save / load models (checkpoints are save in "model_dir/model_name")
        :param batch_samples_per_class: number of samples in one batch sampled from one class label
        :param sparse_seq_dim: learned embedding size for sparse sequential features
        :param sparse_dim: learned embedding size for sparse features
        :param train: if True, model will be retrained, otherwise loaded from existed checkpoint
        :param train_split: fraction of train set (the rest will be used to control validation loss)
        :param num_epochs: number of epochs
        :param encoder_num_units: number of hidden units in encoder
        :param encoder_num_layers: number of hidden layers in encoder
        :param restore_weights_mode:
                    "last" - use model that is saved after all epochs passed
                    "best" - use model that exposes best validation statistics
        :param raise_on_inconsistent_input: if True, raise an error if input feature space has changed
        :param oov:
                    "ignore": don't append any index for OOV
                    "index": process OOV with special token
                    "error": raise an error when OOV is met
        :param num_classes_in_batch: number of unique classes in batch (all classes by default)
        :param loss: dict of loss params of the form {"name": "<loss name>", "param1": param1, ...}
                    <loss_name>:
                        "triplet": triplet loss
                        "margin": margin based loss
                        "logloss": probabilistic log loss
                        "topksoftmax": probabilistic log loss with top-k neighbors selection
        :param class_sampling: class labels sampling strategy when "num_classes_in_batch" was set
                    "uniform": uniform sampling
                    "freq": sampling following number of examples presented in each class
                    "sqrt_freq": sampling following sqrt(number) of examples presented in each class
        :param lr: initial learning rate
        :param lr_decay: multiply lr by this value every lr_decay_steps updates (set None to run without lr scheduler)
        :param lr_decay_steps: number of updates after then lr will be modified
        :param model_name: subdirectory name to save checkpoints / logs
        :param logdir: directory to save logs (VINS temp dir is used by default)
        :param l2norm: normalize output embeddings to unit length
        :param checkpoint_freq_in_batches: how often (in terms of updates) model checkpoints will be saved
        :param kwargs:
        """

        loss = loss or MetricLearningConfig.loss_params
        encoder_output_dense_layers = encoder_output_dense_layers or MetricLearningConfig.encoder_output_dense_layers

        self._embeddings_map_ids = None
        if embeddings_map_key is not None and features_extractor_info is not None:
            self._embeddings_map_ids = features_extractor_info.get(embeddings_map_key)

        assert restore_weights_mode in ('best', 'last')
        assert loss['name'] in ('triplet', 'margin', 'logloss', 'topksoftmax')
        assert class_sampling in ('uniform', 'freq', 'sqrt_freq')
        assert oov in ('ignore', 'index', 'error')
        assert encoder_pooling in ('last', 'max')

        super(MetricLearningFeaturesPostProcessor, self).__init__()

        self._logdir = logdir
        self._model_dir = model_dir

        self._config = MetricLearningConfig(
            model=MetricLearningModel,

            batch_samples_per_class=batch_samples_per_class,
            num_classes_in_batch=num_classes_in_batch,
            class_sampling=class_sampling,
            train_split=train_split,
            exclude_labels_from_validation=exclude_intents_from_validation,
            valid_num_classes=valid_num_classes,
            valid_samples_per_class=valid_samples_per_class,
            batch_generator_njobs=batch_generator_njobs,
            batch_generator_queue_maxsize=batch_generator_queue_maxsize,

            sparse_seq_output_size=sparse_seq_dim,
            sparse_output_size=sparse_dim,
            num_epochs=num_epochs,
            run_name=model_name,
            l2norm=l2norm,
            encoder_num_units=encoder_num_units,
            encoder_num_layers=encoder_num_layers,
            learning_rate=lr,
            learning_rate_decay_rate=lr_decay,
            learning_rate_decay_steps=lr_decay_steps,
            checkpoint_freq_in_batches=checkpoint_freq_in_batches,
            encoder_pooling=encoder_pooling,
            encoder_output_dense_layers=encoder_output_dense_layers,
            num_updates=num_updates,
            callbacks=[BatchSchedule(batch_schedule)],
            finetune_embeddings=finetune_embeddings,
            save_weights_mode=save_weights_mode,
            restore_weights_mode=restore_weights_mode,
            save_weights_start=save_weights_start,
            loss_params=OrderedDict(loss),  # if we use just dict, it would be pickled in an unstable way
            transform_batch_size=transform_batch_size)

        self._model = None
        self._train_mode = train

        # for sklearn compatibility
        self.class_labels = None
        self.transition_model = None
        self.intent_infos = None

        self._ensure_helpers_loaded({'tokenizer': {'oov': oov}, 'dense_features_guard': {}})

    def _find_log_dir(self):
        if self._logdir is None or not os.path.exists(self._logdir):
            logdir = os.path.join(vins_temp_dir(), 'metric_learning_logs')
            logger.info('"logdir" is not specified, saving logs into %s by default', logdir)
        else:
            logdir = self._logdir

        return logdir

    def _find_model_dir(self):
        if self._model_dir:
            full_model_dir = get_resource_full_path(self._model_dir)
            if not os.path.exists(full_model_dir):
                raise ValueError('%s not found.' % full_model_dir)
            else:
                return full_model_dir
        else:
            raise IOError('"model_dir" parameter is not specified')

    def _get_info_file(self):
        return os.path.join(self._find_model_dir(), 'metric_learning_info.json')

    def _load_helpers(self):
        with open(self._get_info_file(), 'r') as infile:
            data = json.load(infile)

        self._tokenizer.load(data['tokenizer'])
        self._dense_features_guard.load(data['dense_features_guard'])

    def _save_helpers(self):
        data = {
            'tokenizer': self._tokenizer.save(),
            'dense_features_guard': self._dense_features_guard.save()
        }
        filename = self._get_info_file()
        logger.info('Saving helpers into file {}'.format(filename))
        with open(filename, 'w') as outfile:
            json.dump(data, outfile, indent=4, sort_keys=True)

    def _get_save_config(self):
        return {
            'logdir': self._find_log_dir(),
            'checkpoint_folder': self._find_model_dir()
        }

    def _get_input_sizes_map(self):
        result = {
            'sparse_seq': self._tokenizer.sparse_seq_features_num,
            'sparse': self._tokenizer.sparse_features_num
        }

        result.update(self._dense_features_guard.get_size())

        return result

    @property
    def _dense_seq_embeddings(self):
        return self._embeddings_map_ids.embeddings_matrix if self._embeddings_map_ids is not None else None

    def _fit_no_metric_learning(self, batch_features):
        self._load_helpers()

        self._dense_features_guard.check(batch_features)

        self._ensure_model_loaded(on='fit')

    def _fit_metric_learning_from_scratch(self, batch_features, y, class_labels, transition_model, intent_infos):
        logger.info('FITTING METRIC LEARNING FROM SCRATCH')
        self._dense_features_guard.fit(batch_features)
        self._tokenizer.fit(batch_features, y)

        self._save_helpers()

        batch_features = self._tokenizer.transform(batch_features)
        input_sizes_map = self._get_input_sizes_map()
        train_preprocessor = SampleFeaturesTrainPreprocessor(
            self._config, input_sizes_map, batch_features, y, class_labels,
            transition_model, intent_infos, self._dense_seq_embeddings)

        logger.info('Sparse seq input size: {}\n'
                    'Dense seq input size: {}\n'
                    'Sparse input size: {}\n'
                    'Dense input size: {}\n'.format(input_sizes_map['sparse_seq'], input_sizes_map['dense_seq'],
                                                    input_sizes_map['sparse'], input_sizes_map['dense']))

        self._load_model(train_preprocessor, restore_from_checkpoint=False)
        self._model.train()

        logger.info('Fitting %r completed.', self.__class__.__name__)

    def fit(self, batch_features, y=None, class_labels=None, transition_model=None, intent_infos=None, **kwargs):
        logger.info('Start fitting %r using train mode=%s', self.__class__.__name__, self._train_mode)

        if self._train_mode == TrainMode.METRIC_LEARNING_FROM_SCRATCH:
            self._fit_metric_learning_from_scratch(batch_features, y, class_labels, transition_model, intent_infos)
        elif self._train_mode == TrainMode.NO_METRIC_LEARNING:
            self._fit_no_metric_learning(batch_features)
        else:
            raise ValueError('Unknown train mode {}'.format(self._train_mode))

        # this flag may cause the model to refuse to retrain for the second time - but it is planned this way
        self._train_mode = TrainMode.NO_METRIC_LEARNING
        return self

    def _ensure_helpers_loaded(self, params, load=False):
        self._tokenizer = SparseIndexerFeaturesPostProcessor(**params['tokenizer'])
        self._dense_features_guard = DenseFeaturesGuard(**params['dense_features_guard'])

        if load:
            self._load_helpers()

    def _load_model(self, preprocessor, restore_from_checkpoint=True, for_train=True):
        save_config = self._get_save_config()

        self._model = (Trainer if for_train else Applier)(self._config, save_config, preprocessor)

        if restore_from_checkpoint and not self._model.restore_weights(mode=self._config.restore_weights_mode):
            raise ValueError('Unable to restore weights from %s' % save_config['checkpoint_folder'])

    def _ensure_model_loaded(self, on, for_train=False):
        load_on_call = get_bool_setting('LOAD_TF_ON_CALL')

        if any((
            on == 'call' and load_on_call and self._model is None,
            on != 'call' and not load_on_call
        )):
            logger.info('Loading %s model on %s...', self.__class__.__name__, on)

            transform_preprocessor = SampleFeaturesPreprocessor(self._dense_seq_embeddings, self._get_input_sizes_map())

            self._load_model(transform_preprocessor, restore_from_checkpoint=True, for_train=for_train)

            assert self._model is not None

    def __getstate__(self):
        d = dict(self.__dict__)

        for key in ['_model', '_tokenizer', '_dense_features_guard']:
            d[key] = None

        return d

    def __setstate__(self, state):
        logger.info('Deserializing %s', self.__class__.__name__)

        self.__dict__.update(state)

        self._ensure_helpers_loaded({'tokenizer': {'oov': None}, 'dense_features_guard': {}}, load=True)
        self._ensure_model_loaded(on='unpickle')

    def make_applier(self):
        self._model.make_applier()

    def transform(self, batch_features):
        result = []

        batch_features = self._tokenizer.transform(batch_features)
        self._ensure_model_loaded(on='call')
        nsplits = int(np.ceil(len(batch_features) / float(self._config.transform_batch_size)))

        for i, batch_i in enumerate(np.array_split(batch_features, nsplits)):
            if len(batch_i):
                result.append(self._model.encode(batch_i))

        return np.vstack(result), map(itemgetter(0), batch_features)
