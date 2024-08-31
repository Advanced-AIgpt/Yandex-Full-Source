import codecs
import logging
import math
import os
from enum import Enum
from operator import attrgetter

import attr
import numpy as np

import yt.wrapper as yt

logger = logging.getLogger(__name__)

OOV_TOKEN = '[PAD]'


class ModelMode(str, Enum):
    BINARY = 'binary'
    MULTICLASS = 'multiclass'
    MULTILABEL = 'multilabel'


@attr.s
class Example(object):
    token_ids = attr.ib()
    label = attr.ib()
    weight = attr.ib(default=None)
    row = attr.ib(default=None)

    def __len__(self):
        return len(self.token_ids)


@attr.s
class DatasetConfig(object):
    text_column = attr.ib()
    label_column = attr.ib()
    true_intent_column = attr.ib(default='')
    valid_table = attr.ib(default='')
    train_table = attr.ib(default='')
    encode_labels = attr.ib(default=False)
    output_table = attr.ib(default=None)
    weight_column = attr.ib(default=None)
    table_to_predict = attr.ib(default=None)
    max_tokens = attr.ib(default=None, type=int)
    tokens_column = attr.ib(default=None)

    @classmethod
    def load(cls, json_config):
        return cls(**json_config)


class BatchGenerator(object):
    def __init__(self, examples, batch_size, seed, sqrt_weight_normalizer=None, shuffle_dataset=False, use_weights=False):
        self._examples = np.array(examples)
        self._batch_size = batch_size
        self._use_weights = use_weights
        self._sqrt_weight_normalizer = sqrt_weight_normalizer
        self._batches_count = int(math.ceil(float(len(self._examples)) / self._batch_size))
        self._generator = self._iterate_batches(self._examples)
        self._shuffle_dataset = shuffle_dataset

        if seed is not None:
            np.random.seed(seed)

    def _iterate_batches(self, examples):
        indices = np.arange(len(examples))

        while True:
            if self._shuffle_dataset:
                np.random.shuffle(indices)

            for i in range(self._batches_count):
                batch_begin = i * self._batch_size
                batch_end = min((i + 1) * self._batch_size, len(examples))
                batch_indices = indices[batch_begin: batch_end]

                yield self._build_batch(examples[batch_indices])

    def _build_batch(self, samples):
        def batchify_matrix(get_field, max_length, dtype):
            tensor = np.zeros((len(samples), max_length), dtype=dtype)

            for sample_id, sample in enumerate(samples):
                data = get_field(sample)
                tensor[sample_id, :len(data)] = data

            return tensor

        lengths = np.array([len(sample) for sample in samples])
        max_length = max(lengths)

        batch = {
            'lengths': lengths,
            'token_ids': batchify_matrix(attrgetter('token_ids'), max_length, np.int64),
            'labels': np.array([sample.label for sample in samples]),
            'rows': [sample.row for sample in samples]
        }

        if self._use_weights:
            weights = []
            for sample in samples:
                if isinstance(sample.weight, list):
                    weights.append([round(math.sqrt(w)) if self._sqrt_weight_normalizer else w for w in sample.weight])
                else:
                    weights.append(round(math.sqrt(sample.weight)) if self._sqrt_weight_normalizer else sample.weight)
            batch['weights'] = np.array(weights)

        return batch

    def __len__(self):
        return self._batches_count

    def __iter__(self):
        return self

    def next(self):
        return self._generator.next()


def setup_yt():
    yt.config['proxy']['url'] = 'hahn'
    yt.config['read_parallel']['enable'] = True
    yt.config['read_parallel']['max_thread_count'] = 32

    yt.config['write_parallel']['enable'] = True
    yt.config['write_parallel']['max_thread_count'] = 64
    yt.config['write_parallel']['unordered'] = True


def read_data(input_table, dataset_config, model_config, word_to_index, allow_high_unknown_token_ratio, is_train=True):
    unknown_token_count, token_count, filtered_row_count = 0., 0, 0
    examples = []
    for row in yt.read_table(input_table):

        if dataset_config.tokens_column is None:
            tokens = row[dataset_config.text_column].decode('utf8').split()
        else:
            tokens = [t.decode('utf8') for t in row[dataset_config.tokens_column]]

        if is_train and dataset_config.max_tokens is not None and len(tokens) > dataset_config.max_tokens:
            filtered_row_count += 1
            continue

        label = None
        if is_train:
            label = row[dataset_config.label_column]

        weight = None
        if is_train and dataset_config.weight_column:
            weight = row[dataset_config.weight_column]

        token_ids = [word_to_index.get(token, word_to_index[OOV_TOKEN]) for token in tokens]
        unknown_token_count += sum(token_id == word_to_index[OOV_TOKEN] for token_id in token_ids)
        token_count += len(token_ids)

        if isinstance(label, list) and model_config.trainer.mode != ModelMode.MULTILABEL:
            if not isinstance(weight, list):
                weight = [weight for _ in range(len(label))]
            intent = row[dataset_config.true_intent_column]
            for single_label, single_weight, single_intent in zip(label, weight, intent):
                row_unq = row.copy()
                row_unq[dataset_config.label_column] = single_label
                row_unq[dataset_config.weight_column] = single_weight
                row_unq[dataset_config.true_intent_column] = single_intent
                examples.append(Example(token_ids, single_label, single_weight, row_unq))
        else:
            examples.append(Example(token_ids, label, weight, row))

    logger.info('Filtered out by max_tokens examples: %s', filtered_row_count)

    if token_count == 0:
        raise ValueError("All tokens were filtered out by max_tokens param")

    logger.info('Unknown tokens ratio: %s', unknown_token_count / token_count)
    assert unknown_token_count / token_count < 0.01 or allow_high_unknown_token_ratio

    return examples


def encode_labels(train_examples, val_examples):
    label_to_index = {}
    for example in train_examples:
        if example.label not in label_to_index:
            label_to_index[example.label] = len(label_to_index)
        example.label = label_to_index[example.label]

    for example in val_examples:
        if example.label not in label_to_index:
            logger.info('Label %s is missing in train set', example.label)
            label_to_index[example.label] = len(label_to_index)
        example.label = label_to_index[example.label]

    return label_to_index


def load_embeddings(special_tokens, embeddings_dir):
    special_tokens = [OOV_TOKEN] + special_tokens

    embeddings_matrix = np.load(os.path.join(embeddings_dir, 'embeddings.npy'))
    embeddings_matrix = np.concatenate((np.zeros((len(special_tokens), embeddings_matrix.shape[-1])), embeddings_matrix), axis=0)

    with codecs.open(os.path.join(embeddings_dir, 'embeddings.dict'), encoding='utf8') as f:
        index_to_word = special_tokens + [line.rstrip() for line in f]
        word_to_index = {}

        for index, word in enumerate(index_to_word):
            if word in word_to_index:
                raise ValueError('Duplicate embeddings for token {}'.format(word))

            word_to_index[word] = index

    return embeddings_matrix, index_to_word, word_to_index
