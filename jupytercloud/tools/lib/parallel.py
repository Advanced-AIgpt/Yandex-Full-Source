# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import tqdm
import functools
import random

from multiprocessing.pool import ThreadPool as BaseTrheadPool, TimeoutError


def get_future_value(future, timeout=None):
    # Если у future-a вызвать get без timeout-а, то программа перестает
    # реагировать на KeyboardInterrupt до завершения.
    # Вроде как, это баг питона.
    # Подробнее: https://stackoverflow.com/a/1408476
    if timeout is not None:
        return future.get(timeout)

    while True:
        try:
            result = future.get(24 * 60 * 60)
            break
        except TimeoutError:
            pass

    return result


class ThreadPool(BaseTrheadPool):
    """
    Специальный ThreadPool с перегруженными apply и map.
    В оригинальном ThreadPool их вызов наглухо блокирует программу так,
    что она перестает реагировать на KeyboardInterrupt до завершения созданных
    тредов.
    Здесь это поправлено.

    """

    def apply(self, func, args=(), kwds={}, timeout=None):
        future = self.apply_async(func, args, kwds)
        return get_future_value(future, timeout)

    def map(self, func, iterable, chunksize=None, timeout=None):
        future = self.map_async(func, iterable, chunksize)
        return get_future_value(future, timeout)


def process(function, iterable, threads, random_order=True):
    iterable = list(iterable)
    if random_order:
        random.shuffle(iterable)

    if threads is None or threads > 1:
        pool = ThreadPool(processes=threads)
        result = pool.map(function, iterable)
        pool.close()
    else:
        result = [function(item) for item in iterable]

    return dict(zip(iterable, result))


def process_with_progress(function, iterable, threads, random_order=True):
    progress_bar = tqdm.tqdm(total=len(iterable))

    @functools.wraps(function)
    def _function(item):
        result = function(item)
        progress_bar.update(1)
        return result

    with progress_bar:
        return process(_function, iterable, threads)
