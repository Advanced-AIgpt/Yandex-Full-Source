# coding: utf-8

from abc import ABCMeta, abstractmethod


def _hashable(item):
    try:
        hash(item)
    except TypeError:
        return False
    return True


class BaseCache(object):
    __metaclass__ = ABCMeta
    UPDATABLE = True

    @abstractmethod
    def update(self, inputs, outputs):
        pass

    def __getitem__(self, item):
        if _hashable(item) and self.__contains__(item):
            return self._get(item)

    @abstractmethod
    def __contains__(self, item):
        pass

    @abstractmethod
    def _get(self, item):
        pass

    def check_consistency(self, *args, **kwargs):
        pass


class IterableCache(BaseCache):
    @abstractmethod
    def iterate_all(self):
        pass

    def collect_all(self):
        return dict(self.iterate_all())
