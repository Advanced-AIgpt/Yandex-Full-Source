# coding: utf-8
from __future__ import unicode_literals, absolute_import


def _sample_with_rejection(sample_size, num_total, rng):
    used = set()
    result = []
    while len(result) < sample_size:
        sample = rng.randint(0, num_total)
        if sample not in used:
            result.append(sample)
            used.add(sample)
    return result


def isample_tuples_from_groups(sample_size, group_sizes, rng):
    """
    Iterator version of sampling without replacement tuples of `len(group_sizes)`.
    Each element in tuples[i] will be from `0` to `group_sizes[i] - 1`
    Example:
        >>> list(isample_tuples_from_groups(2, [1, 3, 2]))
        [(0, 2, 0), (0, 1, 1)]
    Args:
    :param sample_size:
    :param group_sizes:
    :param rng: np.random.RandomState()
    :return:
    """
    total_num_tuples = reduce(lambda x, y: x * y, group_sizes)
    assert sample_size <= total_num_tuples
    if total_num_tuples == 0:
        return

    max_rejection_prob = (sample_size - 1.0) / total_num_tuples
    if max_rejection_prob > 0.5:
        tuple_ids = rng.choice(total_num_tuples, sample_size, replace=False)
    else:
        tuple_ids = _sample_with_rejection(sample_size, total_num_tuples, rng)

    for tuple_id in tuple_ids:
        result = []
        for size in reversed(group_sizes):
            result.append(tuple_id % size)
            tuple_id //= size
        yield tuple(reversed(result))


def sample_tuples_from_groups(sample_size, group_sizes, rng):
    """
    Sample without replacement tuples of `len(group_sizes)`.
    Each element in tuples[i] will be from `0` to `group_sizes[i] - 1`
    Example:
        >>> sample_tuples_from_groups(2, [1, 3, 2])
        [(0, 2, 0), (0, 1, 1)]
    Args:
    :param sample_size:
    :param group_sizes:
    :param rng: np.random.RandomState()
    :return:
    """
    return list(isample_tuples_from_groups(sample_size, group_sizes, rng))
