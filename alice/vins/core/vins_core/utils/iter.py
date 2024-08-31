# coding: utf-8
from __future__ import unicode_literals, absolute_import

import logging


logger = logging.getLogger(__name__)


def first_of(iterable, default=None):
    """
    Returns the first item of `iterable`
    If `iterable` is empty returns `default`

    >>> first_of([1, 2, 3]])
    1
    >>> first_of([])
    None
    >>> first_of([], 8)
    8
    """

    return next(iter(iterable), default)


def all_chains(lists):
    """
    Take a list of lists and generate all possible chains of their elements.

    >>> list(all_chains([['aa', 'ab', 'ac'], ['ba', 'bb']]))
    [['aa', 'ba'], ['aa', 'bb'], ['ab', 'ba'], ['ab', 'bb'], ['ac', 'ba'], ['ac', 'bb']]
    """
    if len(lists) == 0:
        yield []
    else:
        for item in lists[0]:
            for comb in all_chains(lists[1:]):
                yield [item] + comb
