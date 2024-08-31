#!/usr/bin/env python
# encoding: utf-8
"""
Операции со списками таблиц
"""
from traceback import print_exc
from time import asctime


def run_for_date_range(from_date, to_date, func, *args, **kwargs):
    """
    Применение функции func ко всем датам в интервале from_date, to_date
    Все доп.аргументы будут проброшены в функцию. Подразумевается, что дата идёт первым аргументом.
    :param str|datetime from_date:
    :param str|datetime to_date:
    :param Callable func:
    :param args:
    :param kwargs:
    :return:
    """
    from nile.api.v1.datetime import date_range

    for date in date_range(from_date, to_date):
        print('start "%s" at %s' % (date, asctime()))
        try:
            func(date, *args, **kwargs)
        except Exception:
            print_exc()
    print('finished at %s' % asctime())
