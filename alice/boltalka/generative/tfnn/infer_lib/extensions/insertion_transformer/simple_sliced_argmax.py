import numpy as np
import tensorflow as tf
from tfnn.ops.gumbel import gumbel_noise

from .util import take_along_axis


def categorical_sampler(values, k, replacement=True):
    """
    Sample from categorical distribution specified by values k times
    :param values: unnormalized log-probabilities Union[[distribution_size], [n_distributions, distribution_size]]
    :param k: number of samples
    :param replacement:
    :return: Union[[distribution_size], [n_distributions, k]], Union[[distribution_size], [n_distributions, k]]
    """

    def _sample_without_replacement(values_2d_, k_):
        """
        Courtesy of https://github.com/tensorflow/tensorflow/issues/9260#issuecomment-437875125
        """
        delta_ = gumbel_noise(tf.shape(values_2d_))
        _, indices_2d_ = tf.nn.top_k(values_2d_ + delta_, k_)
        return indices_2d_

    def _categorical_sampler(values_2d_, k_, replacement_):
        """
        Sample helper that perform sampling if and only k greater thar 0
        :param values_2d_:
        :param k_:
        :return:
        """
        if replacement_:
            # TODO: rewrite with tf.random.categorical (tf.__version__ >= 1.13)
            max_values_ = tf.reduce_max(values_2d_, axis=-1)
            # Subtract maximum value for numeric stability on GPU
            values_2d_safe_ = values_2d_ - max_values_[:, None]
            indices_2d_ = tf.multinomial(values_2d_safe_, k_, output_dtype=tf.int32)
        else:
            indices_2d_ = _sample_without_replacement(values_2d_, k_)
        # If all values_2d_[i, :] equal to -inf than exchange out-of-range indices_2d_ to valid positions
        indices_2d_ = tf.where(
            tf.greater_equal(indices_2d_, tf.shape(values_2d_)[1]),
            tf.tile(tf.range(tf.shape(indices_2d_)[1])[None, :], [tf.shape(indices_2d_)[0], 1]),
            indices_2d_
        )
        values_2d_ = take_along_axis(values_2d_, indices_2d_)
        return values_2d_, indices_2d_

    # If values is 1-D tensor then reshape it in 2-D tensor
    values_2d = tf.cond(
        tf.equal(tf.rank(values), 2),
        lambda: values, lambda: values[None, :]
    )
    # Check if k is greater than 0 to support border case
    values_2d, indices_2d = tf.cond(
        tf.equal(k, 0),
        lambda: (tf.zeros([tf.shape(values_2d)[0], 0]), tf.zeros([tf.shape(values_2d)[0], 0], dtype=tf.int32)),
        lambda: _categorical_sampler(values_2d, k, replacement)
    )
    # Make output consistent with input
    values, indices = tf.cond(
        tf.equal(tf.rank(values), 2),
        lambda: (values_2d, indices_2d),
        lambda: (tf.squeeze(values_2d, [0]), tf.squeeze(indices_2d, [0]))
    )
    return values, indices


def _process_slice(values, slice_start, slice_end, k, mask, sampler, top_k=None, *args, **kwargs):
    sliced_values = values[slice_start:slice_end]
    sliced_mask = mask[slice_start:slice_end] if mask is not None else tf.ones_like(sliced_values, dtype=tf.bool)
    flattened_mask = tf.reshape(sliced_mask, shape=(-1,))
    flattened_values = tf.reshape(sliced_values, shape=(-1,))
    flattened_values = tf.where(flattened_mask, flattened_values, tf.fill(tf.shape(flattened_mask), -np.inf))
    top_k = top_k if top_k is not None else tf.shape(flattened_values)[-1]
    min_top_k_values = tf.reduce_min(tf.nn.top_k(flattened_values, k=top_k)[0], axis=-1, keepdims=True)
    flattened_values = tf.where(
        tf.greater_equal(flattened_values, min_top_k_values),
        flattened_values, tf.fill(tf.shape(flattened_values), -np.inf)
    )
    n_allowed = tf.count_nonzero(tf.is_finite(flattened_values), axis=-1, dtype=tf.int32)
    values, indices = sampler(flattened_values, tf.minimum(k, n_allowed), *args, **kwargs)
    values = tf.reshape(values, shape=(-1, 1))
    indices = tf.reshape(indices, shape=(-1, 1))
    values = tf.pad(values, paddings=[[0, tf.maximum(0, k - n_allowed)], [0, 0]], constant_values=-np.inf)
    indices = tf.pad(indices, paddings=[[0, tf.maximum(0, k - n_allowed)], [0, 0]], constant_values=-1)
    values = values[:, 0]
    indices = indices[:, 0]
    return values, indices


def _sliced_helper(values, slices, k, mask, sampler, *args, **kwargs):
    slice_starts = slices
    slice_ends = tf.concat([slices[1:], [tf.shape(values)[0]]], axis=0)
    sliced = tf.map_fn(
        lambda x: _process_slice(values, x[0], x[1], k, mask, sampler, *args, **kwargs),
        (slice_starts, slice_ends),
        dtype=(tf.float32, tf.int32)
    )
    return sliced


def sliced_sample(values, slices, k, mask, *args, **kwargs):
    return _sliced_helper(values, slices, k, mask=mask, sampler=categorical_sampler, *args, **kwargs)


def sliced_argmax(values, slices, k, mask=None, *args, **kwargs):
    """
    Computes top-k of values in each slice.
    :param values: matrix of shape [m,n]
    :param slices: vector of shape [m] containing start indices for each slice.
    :param k: take this many elements with largest values from each slice
    :returns: batch_scores,batch_indices:
        - batch_scores[m,k] - top-beam_size values from logP corresponding to
        - batch_indices[m,k] - indices of batch_scores in each respective slice (first value in each slice has index 0!)
    For any slice contains less than k elements, batch_scores would be padded with -inf, batch_indices - with -1
    If values.shape[1] != 1, batch_indices will still be 1-dimensional, satisfying the following property:
        - batch_scores,batch_indices = sliced_argmax(values,slices,k)
        - start, end = slices[i], slices[i+1]
        - tf.equals(batch_scores == tf.reshape(values[start:end,:],[-1])[batch_indices])  #this is True for all indices
    Examples
    --------
    >>> logp = tf.constant(np.array([[1, 2,   3, 4, 5,   6],
                                     [6, 5,   4, 3, 2,   1]],'float32').T)
    >>> slices = tf.constant([0,2,5])
    >>> best_scores, best_indices = sliced_argmax(logp,slices,tf.constant(4))
    >>> print('scores:\n%s\nindices:\n%s'%(best_scores.eval(), best_indices.eval()))
    scores:
    [[  6.   5.   2.   1.]
     [  5.   4.   4.   3.]
     [  6.   1. -inf -inf]]
    indices:
    [[ 1  3  2  0]
     [ 4  1  2  3]
     [ 0  1 -1 -1]]
    """
    assert values.shape.ndims in (None, 2), "values must be 2-dimensional"
    assert slices.shape.ndims in (None, 1), "slices must be 1-dimensional"

    return _sliced_helper(values, slices, k, mask=mask, sampler=tf.nn.top_k, *args, **kwargs)
