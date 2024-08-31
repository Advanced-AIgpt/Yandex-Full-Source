# coding: utf-8
from __future__ import unicode_literals

import inspect
import numpy as np

from collections import defaultdict
from sklearn.base import BaseEstimator
from sklearn.pipeline import Pipeline


def _check_estimator_methods(estimator):
    return all((
        hasattr(estimator, 'fit'),
        hasattr(estimator, 'transform'),
        hasattr(estimator, 'get_params'),
        hasattr(estimator, 'set_params')
    ))


def get_estimator_name(estimator):
    if inspect.isclass(estimator) and issubclass(estimator, BaseEstimator):
        return estimator.__name__.lower()
    elif isinstance(estimator, BaseEstimator) or _check_estimator_methods(estimator):
        return estimator.__class__.__name__.lower()
    else:
        raise TypeError('"estimator" argument should be class definition'
                        ' or sklearn.base.BaseEstimator instance')


def name_estimators(estimators):
    """Generate names for estimators."""

    names = map(get_estimator_name, estimators)
    namecount = defaultdict(int)
    for est, name in zip(estimators, names):
        namecount[name] += 1

    namecount = {k: v for k, v in namecount.iteritems() if v != 1}

    for i in reversed(range(len(estimators))):
        name = names[i]
        if name in namecount:
            names[i] += "-%d" % namecount[name]
            namecount[name] -= 1

    return zip(names, estimators)


def make_pipeline(*steps):
    return Pipeline(name_estimators(steps))


def encode_labels(values, uniques=None, encode=False):
    """ We use this encoder instead of default sklearn label encoder, because it does not assume labels are sorted """
    if uniques is None:
        uniques = sorted(set(values))
        uniques = np.array(uniques, dtype=values.dtype)
    if encode:
        table = {val: i for i, val in enumerate(uniques)}
        try:
            encoded = np.array([table[v] for v in values])
        except KeyError as e:
            raise ValueError("y contains previously unseen labels: %s" % str(e))
        return uniques, encoded
    else:
        return uniques
