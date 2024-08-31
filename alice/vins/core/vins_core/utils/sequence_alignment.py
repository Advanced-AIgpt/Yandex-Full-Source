# -*- coding: utf-8 -*-

import numpy as np


def _build_lcs_matrix(first_sequence, second_sequence):
    lcs_matrix = np.zeros((len(first_sequence) + 1, len(second_sequence) + 1))
    for i in xrange(len(first_sequence)):
        for j in xrange(len(second_sequence)):
            if first_sequence[i] == second_sequence[j]:
                lcs_matrix[i + 1, j + 1] = lcs_matrix[i, j] + 1
            else:
                lcs_matrix[i + 1, j + 1] = max(lcs_matrix[i, j + 1], lcs_matrix[i + 1, j])
    return lcs_matrix


def align_sequences(source_sequence, target_sequence):
    """Returns indices of tokens from source_sequence in target_sequence, -1 for unaligned tokens"""

    lcs_matrix = _build_lcs_matrix(source_sequence, target_sequence)
    alignment = np.ones(len(source_sequence), dtype=np.int) * -1

    source_seq_index, target_seq_index = len(source_sequence) - 1, len(target_sequence) - 1
    while source_seq_index >= 0 and target_seq_index >= 0:
        if source_sequence[source_seq_index] == target_sequence[target_seq_index]:
            alignment[source_seq_index] = target_seq_index
            source_seq_index, target_seq_index = source_seq_index - 1, target_seq_index - 1
        elif lcs_matrix[source_seq_index, target_seq_index + 1] > lcs_matrix[source_seq_index + 1, target_seq_index]:
            source_seq_index -= 1
        else:
            target_seq_index -= 1

    return alignment
