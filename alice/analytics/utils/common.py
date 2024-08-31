#!/usr/bin/env python
# -*-coding: utf8 -*-

import os
from nile.api.v1 import with_hints


def set_last_table_on_finish(date_parameter_name, path_parameter_name, path_parameter_default):
    def wrapper(func):
        def wrapped(*args, **kwargs):
            job = func(*args, **kwargs)
            set_last_table_symlink(job.driver, kwargs.get(date_parameter_name), kwargs.get(path_parameter_name, path_parameter_default))
        return wrapped
    return wrapper


def mapper_wrapper(cls):
    def wrapped(*args, **kwargs):
        instance = cls(*args, **kwargs)
        return with_hints(instance.output_schema)(instance)

    return wrapped


def get_dict_path(dct, path, default=None, expected_type=None, convert_type=None):
    try:
        for key in path:
            dct = dct[key]
    except (KeyError, TypeError):
        return default

    if expected_type and not isinstance(dct, expected_type):
        return default

    if convert_type and dct is not None:
        try:
            return convert_type(dct)
        except TypeError:
            return default

    return dct


def set_last_table_symlink(driver, date, dir_path, set_last_day_attribute=True):
    """
    Если посчитанная табличка - свежая, ставим на нее симлинку last
    """
    # date = options.dates[0]
    dir_list = driver.list(dir_path)

    if 'last' in dir_list:
        dir_list.remove('last')

    last_date = max(dir_list)
    target_path = os.path.join(dir_path, date)
    link_path = os.path.join(dir_path, 'last')

    if date >= last_date:
        driver.client.yt_client.link(target_path, link_path, recursive=False, ignore_existing=False, force=True)

    if set_last_day_attribute:
        driver.set_attribute(os.path.join(dir_path, date), 'last-day', date)
