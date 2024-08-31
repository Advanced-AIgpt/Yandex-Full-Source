import numpy as np
import multiprocessing as mp
import logging
import re
from functools import partial
from itertools import izip
from collections import defaultdict

import tensorflow as tf

from vins_core.utils.multiprocess_queue import MultiprocessIterator


logger = logging.getLogger(__name__)


def sparse_tuple_from(sequences, dtype=np.int32):
    indices = []
    values = []

    for n, seq in enumerate(sequences):
        indices.extend(zip([n] * len(seq), range(len(seq))))
        values.extend(seq)

    indices = np.asarray(indices, dtype=np.int32)
    values = np.asarray(values, dtype=dtype)
    shape = np.asarray([len(sequences), indices.max(0)[1] + 1], dtype=np.int32)

    return tf.SparseTensorValue(indices=indices, values=values, dense_shape=shape)


def align_list(l, fill):
    max_len = max(map(len, l))
    result = []

    for row in l:
        result.extend(row + [fill] * (max_len - len(row)))

    return result


class SampleMasker(object):

    def __init__(self, labels, transition_model=None, intent_infos=None, exclude_labels_from_validation=None):
        self._labels = np.array(labels)
        self._transition_model = transition_model
        self._exclude_labels_from_validation = exclude_labels_from_validation

        self._intent_infos = intent_infos

    def _common_ancestor_exists(self, label_i, label_j):
        for prev_label in self._labels:
            if self._transition_model[prev_label, label_i] > 0 and self._transition_model[prev_label, label_j] > 0:
                return True
        return False

    def _get_transition_model_mask(self):
        n = len(self._labels)
        mask = np.ones(shape=(n, n), dtype=np.bool)
        if self._transition_model:
            for i in xrange(n):
                for j in xrange(i, n):
                    mask[j, i] = mask[i, j] = self._common_ancestor_exists(self._labels[i], self._labels[j])
        return mask

    def _get_negative_sampling_mask(self, attr_name):
        n = len(self._labels)
        diag = (1 - np.eye(n)).astype(np.bool)

        if self._intent_infos:
            mask = np.zeros((n, n), dtype=np.bool)

            for i, intent_name in enumerate(self._labels):
                regex = getattr(self._intent_infos[intent_name], attr_name)

                if regex is not None:
                    regex = re.compile(regex)
                    j = np.array([bool(regex.match(label)) for label in self._labels])

                    mask[i, j] = True
                else:
                    mask[i, :] = True

            mask = np.logical_and(mask, diag)
        else:
            mask = diag

        return mask

    def get_precomputed_anchors_mask(self):
        if self._intent_infos:
            mask = np.array([self._intent_infos[intent_name].positive_sampling for intent_name in self._labels])
        else:
            mask = np.ones_like(self._labels, dtype=np.bool)

        return mask

    def get_precomputed_negatives_mask(self):
        transition_model_mask = self._get_transition_model_mask()
        negative_sampling_from_mask = self._get_negative_sampling_mask('negative_sampling_from')
        negative_sampling_for_mask = self._get_negative_sampling_mask('negative_sampling_for').T

        return reduce(np.logical_and, (transition_model_mask,
                                       negative_sampling_from_mask,
                                       negative_sampling_for_mask))

    def get_precomputed_recall_mask(self):
        if self._exclude_labels_from_validation:
            return np.array([re.match(self._exclude_labels_from_validation, label) is not None
                             for label in self._labels])
        else:
            return np.ones_like(self._labels, dtype=np.bool)


class SampleFeaturesPreprocessor(object):
    def __init__(self, dense_seq_embeddings, input_sizes_map):
        self.dense_seq_embeddings = dense_seq_embeddings

        self._input_sizes_map = input_sizes_map
        self._input_present = {key: value > 0 for key, value in input_sizes_map.iteritems()}

    @property
    def input_sizes_map(self):
        return self._input_sizes_map

    def _check_dense_seq_ids_consistency(self, dense_seq_ids):
        dense_seq_ids_absent = dense_seq_ids is None
        dense_seq_embeddings_absent = self.dense_seq_embeddings is None
        if dense_seq_ids_absent ^ dense_seq_embeddings_absent:
            raise ValueError('dense_seq_ids absent: {}; dense_seq_embeddings absent: {}. '
                             'They have to be either both present or both absent'.format(dense_seq_ids_absent,
                                                                                         dense_seq_embeddings_absent))

    def extract_data(self, batch_features, for_train):
        data = defaultdict(list)

        for sample_features, encoded_features in batch_features:
            if self._input_present['sparse_seq']:
                data['sparse_seq'].append(encoded_features['sparse_seq'])
            if self._input_present['sparse']:
                data['sparse'].append(encoded_features['sparse'])
            if self._input_present['dense_seq']:
                data['dense_seq'].append(sample_features.dense_seq_matrix())
            if self._input_present['dense']:
                data['dense'].append(sample_features.dense_matrix())

            dense_seq_ids = sample_features.get_dense_seq_ids()
            self._check_dense_seq_ids_consistency(dense_seq_ids)
            if self.dense_seq_embeddings is not None:
                data['dense_seq_ids'].append(dense_seq_ids)

        return self._get_encoder_input_feed(data, for_train)

    def _get_sparse_seq_input_feed(self, batch):
        if not self._input_present['sparse_seq']:
            return {}

        return {
            'sparse_seq_ids': sparse_tuple_from(align_list(batch, [0]))
        }

    def _get_dense_seq_input_feed(self, batch):
        if not self._input_present['dense_seq']:
            return {}

        maxlen = max(map(len, batch))
        dense_seq = np.zeros((len(batch), maxlen, self._input_sizes_map['dense_seq']), dtype=np.float32)
        for i, x in enumerate(batch):
            dense_seq[i, :len(x)] = x

        return {
            'dense_seq': dense_seq
        }

    def _get_sparse_input_feed(self, batch):
        if not self._input_present['sparse']:
            return {}

        return {
            'sparse_ids': sparse_tuple_from(batch)
        }

    def _get_dense_input_feed(self, batch):
        if not self._input_present['dense']:
            return {}

        return {
            'dense': batch
        }

    def _get_dense_seq_ids_input_feed(self, batch, for_train):
        if self.dense_seq_embeddings is None:
            return {}

        if for_train:
            return {
                'dense_seq_ids': sparse_tuple_from(batch)
            }
        else:
            maxlen = max(map(len, batch))
            embeddings = np.zeros((len(batch), maxlen, self.dense_seq_embeddings.shape[-1]), dtype=np.float32)

            for i, ids in enumerate(batch):
                for j, id_ in enumerate(ids):
                    embeddings[i, j] = self.dense_seq_embeddings[id_]

            return {
                'dense_seq_ids_apply': embeddings
            }

    def _get_lengths_feed(self, input):
        lengths = None

        if input['sparse_seq']:
            lengths = map(len, input['sparse_seq'])
        elif input['dense_seq']:
            lengths = map(len, input['dense_seq'])
        elif input['dense_seq_ids']:
            lengths = map(len, input['dense_seq_ids'])
        elif input['sparse']:
            lengths = np.ones(len(input['sparse']))
        elif input['dense']:
            lengths = np.ones(len(input['dense']))

        return {
            'lengths': lengths
        }

    def _get_encoder_input_feed(self, context, for_train):
        feeds = [self._get_lengths_feed(context),
                 self._get_sparse_seq_input_feed(context['sparse_seq']),
                 self._get_dense_seq_input_feed(context['dense_seq']),
                 self._get_sparse_input_feed(context['sparse']),
                 self._get_dense_input_feed(context['dense']),
                 self._get_dense_seq_ids_input_feed(context['dense_seq_ids'], for_train)]

        return {key: value for feed in feeds for key, value in feed.iteritems()}


class SampleFeaturesTrainPreprocessor(SampleFeaturesPreprocessor):
    def __init__(self, config, input_sizes_map, batch_features, batch_labels, class_labels,
                 transition_model, intent_infos, dense_seq_embeddings, rng_seed=42):

        super(SampleFeaturesTrainPreprocessor, self).__init__(dense_seq_embeddings, input_sizes_map)

        self._config = config

        self._batch_features = batch_features
        self._batch_labels = batch_labels

        self._batch_samples_per_class = mp.Value('i', 0)
        self._num_classes_in_batch = mp.Value('i', 0)
        self._valid_num_classes = mp.Value('i', 0)
        self._valid_samples_per_class = mp.Value('i', 0)

        self._train_indices_per_label = {}
        self._train_label_probs = {}
        self._valid_indices_per_label = {}

        self.masker = SampleMasker(
            labels=class_labels,
            transition_model=transition_model,
            intent_infos=intent_infos,
            exclude_labels_from_validation=self._config.exclude_labels_from_validation
        )

        self._num_samples = 0

        label_indices = defaultdict(list)
        for i, y in enumerate(self._batch_labels):
            label_indices[y].append(i)
        self._labels_num = len(label_indices)

        num_valid_samples = 0

        if self._config.train_split < 1:
            rng = np.random.RandomState(rng_seed)

            for label, indices in label_indices.iteritems():
                rng.shuffle(indices)
                num_train_samples = int(self._config.train_split * len(indices))
                self._num_samples += num_train_samples
                num_valid_samples += len(indices) - num_train_samples
                self._train_indices_per_label[label] = indices[:num_train_samples]
                self._valid_indices_per_label[label] = indices[num_train_samples:]
        else:
            self._train_indices_per_label = label_indices
            self._num_samples = len(batch_features)

        for y, y_indices in self._train_indices_per_label.iteritems():
            self._train_label_probs[y] = float(len(y_indices)) / self._num_samples

        self.set_batch_shape(self._config.num_classes_in_batch or self._labels_num,
                             self._config.batch_samples_per_class)

        self._valid_num_classes.value = self._config.valid_num_classes or self._labels_num
        self._valid_samples_per_class.value = self._config.valid_samples_per_class or \
            self._batch_samples_per_class.value

        valid_batch_size = self._valid_num_classes.value * self._valid_samples_per_class.value
        self._num_valid_batches = int(np.ceil(float(num_valid_samples) / valid_batch_size))
        num_valid_calls = self.num_train_batches / self._config.validation_loss_freq

        self._valid_batches_generator = self._iterate_batches_multiprocess(
            self._num_valid_batches * num_valid_calls, self._valid_indices_per_label,
            self._valid_num_classes, self._valid_samples_per_class)

        logger.info('Num classes in batch: %d, batch size=%d', self._num_classes_in_batch.value, self._batch_size)
        logger.info('%d train samples (%d batches)', self._num_samples, self._num_batches)
        logger.info('%d valid samples (%d batches)', num_valid_samples, self._num_valid_batches)

    def set_batch_shape(self, classes, samples):
        with self._num_classes_in_batch.get_lock():
            self._num_classes_in_batch.value = classes
        with self._batch_samples_per_class.get_lock():
            self._batch_samples_per_class.value = samples

        if not classes:
            classes = self._labels_num

        self._batch_size = classes * samples

        self._num_batches = int(np.ceil(float(self._num_samples) / self._batch_size))

    @property
    def batch_size(self):
        return self._batch_size

    def _get_class_probs(self, class_labels):
        cs = self._config.class_sampling

        if cs == 'freq':
            p = np.asarray([self._train_label_probs.get(k, 0) for k in class_labels])
        elif cs == 'sqrt_freq':
            p = np.sqrt([self._train_label_probs.get(k, 0) for k in class_labels])
        elif cs == 'uniform':
            p = np.asarray([self._train_label_probs.get(k, 0) > 0 for k in class_labels], dtype=np.float)
        else:
            raise ValueError('%s type is not understood' % cs)

        return p / np.sum(p)

    def _extract_train_batch(self, from_where, num_classes_in_batch, batch_samples_per_class, rng):
        batch_x, batch_y = [], []
        batch_classes = from_where.keys()
        if num_classes_in_batch == self._labels_num:
            sampled_classes = batch_classes
        else:
            sampled_classes = rng.choice(
                batch_classes, num_classes_in_batch, replace=False, p=self._get_class_probs(batch_classes))

        for y in sampled_classes:
            indices = from_where[y]
            class_sample_size = min(len(indices), batch_samples_per_class)
            if class_sample_size == 1:
                class_sample_size = 2
                replace = True
            else:
                replace = False
            rnd_index = rng.choice(indices, class_sample_size, replace=replace)
            for i in rnd_index:
                batch_x.append(self._batch_features[i])
                batch_y.append(self._batch_labels[i])

        return self.extract_data(batch_x, for_train=True), np.array(batch_y, dtype=np.int32)

    def _iterate_batches_multiprocess(self, num_batches, from_where, num_classes_in_batch, batch_samples_per_class):
        make_gen = partial(self._iterate_batches, num_batches, from_where,
                           num_classes_in_batch, batch_samples_per_class)

        njobs = self._config.batch_generator_njobs

        if njobs is None:
            for item in make_gen():
                yield item
        else:
            for item in MultiprocessIterator(make_gen, njobs, self._config.batch_generator_queue_maxsize, num_batches):
                yield item

    def _iterate_batches(self, num_batches, from_where, num_classes_in_batch, batch_samples_per_class):
        rng = np.random.RandomState()

        for _ in xrange(num_batches):
            context, labels = self._extract_train_batch(
                from_where, num_classes_in_batch.value, batch_samples_per_class.value, rng)
            yield {
                'context': context,
                'labels': labels
            }

    @property
    def num_train_batches(self):
        return self._config.num_updates or self._config.num_epochs * self._num_batches

    def iterate_batches_train(self):
        for item in self._iterate_batches_multiprocess(self.num_train_batches, self._train_indices_per_label,
                                                       self._num_classes_in_batch, self._batch_samples_per_class):
            yield item

    def iterate_batches_test(self):
        for _, item in izip(xrange(self._num_valid_batches), self._valid_batches_generator):
            yield item
