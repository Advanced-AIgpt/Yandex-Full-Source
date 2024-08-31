# coding: utf-8
from __future__ import unicode_literals

import numpy as np
import scipy.sparse

from collections import Iterable
from scipy.sparse import coo_matrix, csr_matrix, csc_matrix

from tensorflow.keras.layers import SimpleRNN, LSTM, GRU
from tensorflow.keras.preprocessing.sequence import pad_sequences as keras_pad_sequences

SPARSE = 'int32'
DENSE = 'float32'


def define_type(sequences):
    if isinstance(sequences, (tuple, list)):
        if not sequences:
            return SPARSE
        if any(is_array_obj(item) for item in sequences):
            return DENSE
        item = filter(None, sequences)
        if not item:
            return SPARSE
        item = item[0]
        if isinstance(item, (int, np.int32)) or isinstance(item, list):
            return SPARSE
        else:
            return None
    elif is_array_obj(sequences):
        return DENSE
    else:
        return None


def RNN(rnn_type):
    if rnn_type == 'rnn':
        return SimpleRNN
    elif rnn_type == 'gru':
        return GRU
    elif rnn_type == 'lstm':
        return LSTM
    else:
        raise TypeError('Unrecognized RNN type: %s' % rnn_type)


def pad_sequences(sequences, maxlen, padding='pre'):
    """
    Extends pad_sequence functionality from keras allowing sequence items to be arbitrary ndarrays
    :param sequences:
    :param maxlen:
    :param padding:
    :return:
    """
    assert padding in ('pre', 'post')
    if isinstance(sequences, (tuple, list)):
        if not sequences or not isinstance(sequences[0], Iterable):
            return keras_pad_sequences([sequences], maxlen, define_type(sequences), padding)
        elif isinstance(sequences[0], (tuple, list)):
            return keras_pad_sequences(sequences, maxlen, define_type(sequences), padding)
        elif isinstance(sequences[0], np.ndarray):
            new_sequences = []
            for items in sequences:
                num_pads = maxlen - items.shape[0]
                pad = np.zeros_like(items[0])
                if num_pads < 0:
                    if padding == 'pre':
                        new_sequences.append(items[-num_pads:])
                    else:
                        new_sequences.append(items[:num_pads])
                else:
                    if padding == 'pre':
                        new_sequences.append(np.concatenate((np.repeat([pad], num_pads, axis=0), items), axis=0))
                    else:
                        new_sequences.append(np.concatenate((items, np.repeat([pad], num_pads, axis=0)), axis=0))
            return np.asarray(new_sequences, dtype=define_type(new_sequences))
        else:
            NotImplementedError('Unsupported input item type: %r' % type(sequences[0]))

    elif isinstance(sequences, np.ndarray):
        pad = np.zeros_like(sequences[0])
        num_pads = maxlen - sequences.shape[0]
        if padding == 'pre':
            if num_pads < 0:
                return sequences[-num_pads:]
            else:
                return np.concatenate((np.repeat([pad], num_pads, axis=0), sequences), axis=0)
        else:
            if num_pads < 0:
                return sequences[:num_pads]
            else:
                return np.concatenate((sequences, np.repeat([pad], num_pads, axis=0)), axis=0)
    elif isinstance(sequences, (coo_matrix, csc_matrix, csr_matrix)):
        num_items, num_dims = sequences.shape
        num_pads = maxlen - num_items
        if num_pads == 0:
            return sequences
        elif num_pads < 0:
            if isinstance(sequences, coo_matrix):
                sequences = sequences.tocsr()
            if padding == 'pre':
                return sequences[-num_pads:]
            else:
                return sequences[:num_pads]
        else:
            pad = coo_matrix((num_pads, num_dims))
            if padding == 'pre':
                new_items = (pad, sequences)
            elif padding == 'post':
                new_items = (sequences, pad)
        return scipy.sparse.vstack(new_items)
    else:
        raise NotImplementedError('Unsupported input sequences type: %r' % type(sequences))


def is_array_obj(x):
    return isinstance(x, (np.ndarray, coo_matrix, csc_matrix, csr_matrix))
