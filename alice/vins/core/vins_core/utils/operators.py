# coding: utf-8
from __future__ import unicode_literals, absolute_import
from operator import eq, gt, lt, itemgetter, attrgetter


class BaseOperator(object):
    def __init__(self, func):
        self._func = func

    def __call__(self, obj):
        return self._func(obj)

    def __eq__(self, value):
        return BaseOperator(lambda _: eq(self(_), value))

    def __gt__(self, value):
        return BaseOperator(lambda _: gt(self(_), value))

    def __lt__(self, value):
        return BaseOperator(lambda _: lt(self(_), value))


def attr(name):
    return BaseOperator(attrgetter(name))


def item(name):
    return BaseOperator(itemgetter(name))
