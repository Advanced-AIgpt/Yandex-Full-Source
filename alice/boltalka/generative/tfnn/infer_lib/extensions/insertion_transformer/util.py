import tensorflow as tf


def take_along_axis(values, indices):
    """
    Gather values in each line with respect indices
    :param values: 2-D tensor [batch_size, n_values]
    :param indices: 2-D tensor [batch_size, n_ids]
    :return: [batch_size, n_ids]
    """
    batch_size = tf.shape(values)[0]
    # diagonal_ids  == [[0, 0], [1, 1], ..., [batch_size - 1, batch_size - 1]]
    diagonal_ids = tf.tile(tf.range(batch_size)[:, None], [1, 2])
    return tf.gather_nd(
        tf.gather(values, indices, axis=1),
        diagonal_ids
    )


def str_to_dtype(name):
    # TODO: change this hack into isinstance(name, tf.Dtype). tf.__version__ >= 1.12
    if not isinstance(name, str):
        return name
    if name == 'float32':
        return tf.float32
    if name in ('float16', 'half'):
        return tf.float16
    raise ValueError("Unexpected value of dtype: " + name)


def sign_back_linear(x):
    with tf.get_default_graph().gradient_override_map({"Sign": "Identity"}):
        return tf.sign(x)


def count_zeros(a, axis=None):
    return tf.size(a) - tf.count_nonzero(a, axis=axis, dtype=tf.int32)


def cartesian_product(a, b):
    tile_a = tf.tile(tf.expand_dims(a, 1), [1, tf.shape(b)[0]])
    tile_a = tf.expand_dims(tile_a, 2)
    tile_b = tf.tile(tf.expand_dims(b, 0), [tf.shape(a)[0], 1])
    tile_b = tf.expand_dims(tile_b, 2)
    return tf.concat([tile_a, tile_b], axis=2)


def squared_dist(a, b):
    row_norms_a = tf.reduce_sum(tf.square(a), axis=1)
    row_norms_a = tf.reshape(row_norms_a, [-1, 1])  # Column vector.
    row_norms_b = tf.reduce_sum(tf.square(b), axis=1)
    row_norms_b = tf.reshape(row_norms_b, [1, -1])  # Row vector.
    return row_norms_a - 2 * tf.matmul(a, tf.transpose(b)) + row_norms_b
