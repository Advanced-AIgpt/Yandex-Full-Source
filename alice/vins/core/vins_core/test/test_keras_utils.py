# coding: utf-8
from __future__ import unicode_literals

import numpy as np


def test_pad_sequences():
    from vins_core.utils.keras_utils import pad_sequences

    # list sequences
    assert np.all(pad_sequences([1, 2], maxlen=4) == np.array([[0, 0, 1, 2]]))
    assert np.all(pad_sequences([1, 2], maxlen=4, padding='post') == np.array([[1, 2, 0, 0]]))
    assert np.all(pad_sequences([], maxlen=4) == np.array([[0, 0, 0, 0]]))

    # list of list sequences
    assert np.all(pad_sequences([[1, 2]], maxlen=4) == np.array([[0, 0, 1, 2]]))
    assert np.all(pad_sequences([[1, 2]], maxlen=4, padding='post') == np.array([[1, 2, 0, 0]]))
    assert np.all(pad_sequences([[]], maxlen=4) == np.array([[0, 0, 0, 0]]))

    # list of ndarrays
    assert np.all(
        pad_sequences([np.array([[1, 1], [2, 2]]), np.array([[3, 3]])], maxlen=4) ==
        np.array([
            [[0, 0], [0, 0], [1, 1], [2, 2]],
            [[0, 0], [0, 0], [0, 0], [3, 3]]
        ])
    )
    assert np.all(
        pad_sequences([np.array([[1, 1], [2, 2]]), np.array([[3, 3]])], maxlen=4, padding='post') ==
        np.array([
            [[1, 1], [2, 2], [0, 0], [0, 0]],
            [[3, 3], [0, 0], [0, 0], [0, 0]]
        ])
    )
    assert np.all(
        pad_sequences([np.array([[1, 1], [2, 2]]), np.array([[3, 3]])], maxlen=1) ==
        np.array([
            [[2, 2]],
            [[3, 3]]
        ])
    )
    assert np.all(
        pad_sequences([np.array([[1, 1], [2, 2]]), np.array([[3, 3]])], maxlen=1, padding='post') ==
        np.array([
            [[1, 1]],
            [[3, 3]]
        ])
    )
    assert np.all(
        pad_sequences([np.random.randn(1, 2, 3, 4), np.random.randn(2, 2, 3, 4)], maxlen=3)[0][0] ==
        np.zeros((2, 3, 4))
    )

    # pure ndarray
    assert np.all(
        pad_sequences(np.array([[1, 2], [3, 4]]), maxlen=3) ==
        np.array([
            [0, 0],
            [1, 2],
            [3, 4]
        ])
    )
