import numpy as np


def approximate_balanced_train_test_split(X, y, test_targets_proportion, random_state=42):
    """
    Splitting dataset so the train split has `1. - test_targets_proportion`
    ratio of each target with an exception that if a target has two or more
    labels validation split has at least one value of that target.

    NOTE: the method does not guarantee to return exact balanced split in
    terms of nor classes neither samples.
    """
    values, counts = np.unique(y, return_counts=True)
    random_state = np.random.RandomState(seed=random_state)

    train_ids, test_ids = [], []
    for value, count in zip(values, counts):
        value_ids = np.where(y == value)[0]

        if count == 1:
            train_ids.extend(value_ids)
        else:
            num_test_samples = max(int(test_targets_proportion * count), 1)
            value_ids = random_state.permutation(value_ids)
            train_ids.extend(value_ids[num_test_samples:])
            test_ids.extend(value_ids[:num_test_samples])

    train_ids, test_ids = np.array(train_ids), np.array(test_ids)

    return X[train_ids], X[test_ids], y[train_ids], y[test_ids]
