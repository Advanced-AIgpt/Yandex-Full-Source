# coding: utf-8
from __future__ import unicode_literals

from collections import defaultdict
from itertools import product

import numpy as np
import pytest

from alice.nlu.py_libs.utils.strings import isnumeric
from alice.nlu.py_libs.utils.sampling import sample_tuples_from_groups, isample_tuples_from_groups


@pytest.mark.parametrize("input, output", [
    ('123', True),
    ('123.456', False),
    ('123,456', False),
    ('xxx123', False),
    ('123xxx', False)
])
def test_isnumeric(input, output):
    assert isnumeric(input) == output


def test_sample_tuples_from_groups():
    rng = np.random.RandomState(123)

    group_sizes = [100, 0, 3]
    assert sample_tuples_from_groups(0, group_sizes, rng) == []

    group_sizes = [1, 3, 1, 2, 4, 1]
    sample_size = reduce(lambda x, y: x * y, group_sizes)
    answer = list(product(*map(range, group_sizes)))
    assert sorted(sample_tuples_from_groups(sample_size, group_sizes, rng)) == answer

    freqs = defaultdict(int)
    sample_size = 3
    num_trials = 10000
    for _ in xrange(num_trials):
        for k in isample_tuples_from_groups(sample_size, group_sizes, rng):
            freqs[k] += 1

    prob = 1.0 / len(answer)
    mean = sample_size * num_trials * prob
    std = (sample_size * num_trials * prob * (1 - prob))**0.5
    for k in answer:
        assert mean - 3 * std <= freqs[k] <= mean + 3 * std
