# coding: utf-8
from __future__ import unicode_literals

import os
import pytest
from glob import glob


@pytest.mark.parametrize("base,ext,result", [
    (
        {'a': 1},
        {'a': 2},
        {'a': 2},
    ),
    (
        {'a': 1},
        {'b': 2},
        {'a': 1, 'b': 2},
    ),
    (
        {'a': [1]},
        {'a': [2]},
        {'a': [1, 2]},
    ),
    (
        {'a': []},
        {'a': [1]},
        {'a': [1]},
    ),
    (
        {'a': 1, 'b': 1},
        {'a': 2},
        {'a': 2, 'b': 1},
    ),
    (
        {'a': None},
        {'a': {'b': 1}},
        {'a': {'b': 1}},
    ),
    (
        {'a': {'b': 1, 'c': {'d': 2}}},
        {'a': {'c': {'d': 3}}},
        {'a': {'b': 1, 'c': {'d': 3}}},
    ),
    (
        {'a': {'b': 1, 'c': {'d': 2}}},
        {'a': {'c': {'e': 3}}},
        {'a': {'b': 1, 'c': {'d': 2, 'e': 3}}},
    ),
])
def test_dict_deep_update(base, ext, result):
    from cit_deploy import deep_dict_update

    deep_dict_update(base, ext)
    assert base == result


def test_load_configs():
    from cit_deploy import load_config

    cur_dir = os.path.dirname(os.path.abspath(__file__))
    for conf_file in glob(os.path.join(cur_dir, '*', '*.yaml')):
        assert load_config(conf_file, '/tmp/vins/sandbox')
