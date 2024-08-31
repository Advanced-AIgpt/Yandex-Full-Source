import tensorflow as tf
import numpy as np


def dense_layer(name, input, output_size, reuse=None, W_0=tf.contrib.layers.xavier_initializer(), b_0=tf.constant_initializer(0.0)):
    with tf.variable_scope(name, reuse=reuse):
        W = tf.get_variable(name='W',
                            shape=[input.get_shape()[-1], output_size],
                            dtype=tf.float32,
                            initializer=W_0)
        b = tf.get_variable(name='b',
                            shape=[output_size],
                            dtype=tf.float32,
                            initializer=b_0)
        return tf.matmul(input, W) + b


def pad_line(line, length, pad):
    return line + [pad] * (length - len(line))


def get_batch(input):
    num_tokens = []
    for line in input:
        num_tokens.append(len(line))
    max_num_tokens = max(num_tokens)

    # empty batch plug
    if max_num_tokens == 0:
        max_num_tokens = 1

    token_ids = []
    for line in input:
        line = pad_line(list(line), max_num_tokens, 0)
        token_ids.append(line)
    return np.array(token_ids), np.array(num_tokens)


def get_sparse_batch(input, dct_size):
    indices = []
    values = []
    batch_map = []
    repr_idx = 0

    for line in input:
        if len(line) == 0:
            batch_map.append(-1)
            continue
        batch_map.append(repr_idx)
        values.extend(line)
        indices.extend(zip([repr_idx]*len(line), range(len(line))))
        repr_idx += 1

    batch_map = np.array(batch_map)
    batch_map[batch_map == -1] = repr_idx

    dense_shape = [repr_idx, dct_size]

    # empty batch plug -- ignore it if batch_map[-1] == 0
    if repr_idx == 0:
        indices = [[0]]
        values = [0]
        dense_shape = [1, dct_size]

    sp_ids = tf.SparseTensorValue(indices, values, dense_shape)

    return sp_ids, batch_map
