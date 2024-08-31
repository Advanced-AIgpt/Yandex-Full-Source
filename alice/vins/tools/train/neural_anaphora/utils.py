# coding: utf-8

import attr
import math
import numpy as np
import logging

from sklearn.metrics import precision_recall_fscore_support

# Imports used only for typing
from typing import List, NoReturn  # noqa: F401

logger = logging.getLogger(__name__)


@attr.s(frozen=True)
class Parameters(object):
    train_data = attr.ib()
    test_data = attr.ib()
    model_name = attr.ib()
    word_emb_dim = attr.ib(default=300)
    encoder_hidden_dim = attr.ib(default=256)
    encoder_num_layers = attr.ib(default=1)
    classifier_hidden_dim = attr.ib(default=256)
    classifier_num_layers = attr.ib(default=2)
    dropout_rate = attr.ib(default=0.5)
    epoch_count = attr.ib(default=5)
    feature_embedding_size = attr.ib(default=8)
    use_segment_embeddings = attr.ib(default=False)
    use_speaker_embeddings = attr.ib(default=False)
    use_distance_embeddings = attr.ib(default=False)
    positive_class_weight = attr.ib(default=10)
    query_softmax_loss = attr.ib(default=False)
    bce_loss = attr.ib(default=True)


@attr.s(frozen=True)
class Example(object):
    tokens = attr.ib()
    token_ids = attr.ib()
    segment_ids = attr.ib()
    positions = attr.ib()
    entity_labels = attr.ib()
    phrase_level_labels = attr.ib()


@attr.s
class Batch(object):
    token_ids = attr.ib()
    segment_ids = attr.ib()
    lengths = attr.ib()
    entity_rows = attr.ib()
    entity_start_positions = attr.ib()
    entity_end_positions = attr.ib()
    pronoun_rows = attr.ib()
    pronoun_start_positions = attr.ib()
    pronoun_end_positions = attr.ib()
    entity_labels = attr.ib()
    phrase_level_labels = attr.ib()


SPECIAL_SYMBOLS = ['[PAD]', '[SEP]', '[BOS]', '[EOS]']
PAD_INDEX = SPECIAL_SYMBOLS.index('[PAD]')
SEP_INDEX = SPECIAL_SYMBOLS.index('[SEP]')
BOS_INDEX = SPECIAL_SYMBOLS.index('[BOS]')
EOS_INDEX = SPECIAL_SYMBOLS.index('[EOS]')

MAX_ENTITY_LEN = 4


def to_matrix(seqs, max_seq_len=None, pad_index=0):
    batch_size = len(seqs)
    seq_len = max(len(seq) for seq in seqs)
    if max_seq_len is not None:
        seq_len = min(seq_len, max_seq_len)

    matrix = np.full((batch_size, seq_len), fill_value=pad_index, dtype=np.int)
    for i, seq in enumerate(seqs):
        matrix[i, :len(seq)] = seq[:seq_len]

    return matrix


class BatchGenerator(object):
    def __init__(self, data, batch_size=32):
        # type: (List[Example], int) -> NoReturn

        self._data = data
        self._batch_size = batch_size
        self._generator = self._iterate_batches()

    def _iterate_batches(self):
        indices = np.arange(len(self._data))
        while True:
            np.random.shuffle(indices)

            for begin_pos in range(0, len(indices), self._batch_size):
                end_pos = min(len(indices), begin_pos + self._batch_size)
                batch_indices = indices[begin_pos: end_pos]

                token_ids, segment_ids, entity_labels, phrase_level_labels = [], [], [], []
                entity_rows, entity_start_positions, entity_end_positions = [], [], []
                pronoun_rows, pronoun_start_positions, pronoun_end_positions = [], [], []
                for row_index, index in enumerate(batch_indices):
                    token_ids.append(self._data[index].token_ids)
                    segment_ids.append(self._data[index].segment_ids)

                    for start_pos, end_pos in self._data[index].positions[:-1]:
                        entity_rows.append(row_index)
                        entity_start_positions.append(start_pos)
                        entity_end_positions.append(end_pos)

                    pronoun_rows.append(row_index)
                    pronoun_start_positions.append(self._data[index].positions[-1][0])
                    pronoun_end_positions.append(self._data[index].positions[-1][1])

                    entity_labels.extend(self._data[index].entity_labels)
                    phrase_level_labels.append(self._data[index].phrase_level_labels)

                batch_lengths = [len(seq) for seq in token_ids]
                assert len(entity_rows) == len(entity_start_positions) == len(entity_end_positions)
                assert len(entity_end_positions) == len(entity_labels)

                assert len(pronoun_rows) == len(pronoun_start_positions) == len(pronoun_end_positions)
                assert len(pronoun_rows) == len(batch_indices)

                assert len(phrase_level_labels) == len(batch_indices)
                assert all(len(el) == 2 for el in phrase_level_labels)

                yield Batch(
                    token_ids=to_matrix(token_ids),
                    segment_ids=to_matrix(segment_ids),
                    lengths=np.array(batch_lengths),
                    entity_rows=np.array(entity_rows),
                    entity_start_positions=np.array(entity_start_positions),
                    entity_end_positions=np.array(entity_end_positions),
                    pronoun_rows=np.array(pronoun_rows),
                    pronoun_start_positions=np.array(pronoun_start_positions),
                    pronoun_end_positions=np.array(pronoun_end_positions),
                    entity_labels=np.array(entity_labels),
                    phrase_level_labels=np.array(phrase_level_labels),
                )

    def __len__(self):
        return int(math.ceil(len(self._data) / self._batch_size))

    def __iter__(self):
        return self._generator


class AccuracyCounter(object):
    def __init__(self):
        self.correct_count = 0.
        self.total_count = 0.

    def update(self, logits, labels):
        predictions = logits > 0.
        self.correct_count += (predictions == labels).sum()
        self.total_count += labels.shape[0]

    def __str__(self):
        return '{:.2%}'.format(self.correct_count / self.total_count)


class F1Counter(object):
    def __init__(self):
        self._logits = []
        self._labels = []

    def update(self, logits, labels):
        self._logits.append(logits)
        self._labels.append(labels)

    def __str__(self):
        logits = np.concatenate(self._logits, 0)
        precision, recall, fscore, _ = precision_recall_fscore_support(
            y_true=np.concatenate(self._labels, 0),
            y_pred=logits > 0.,
            average='binary'
        )

        return 'Precision = {:.2%}, Recall = {:.2%}, F1 = {:.2%}'.format(precision, recall, fscore)
