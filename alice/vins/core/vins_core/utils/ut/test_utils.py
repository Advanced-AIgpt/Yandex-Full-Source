# coding: utf-8
from __future__ import unicode_literals

import logging
import os

import pytest
import math
import numpy as np
import attr

from collections import defaultdict

from vins_core.utils.config import get_setting, get_bool_setting, DEFAULT_VINS_PREFIX
from vins_core.utils.datetime import parse_tz
from vins_core.utils.data import delete_keys_from_dict, vins_temp_dir
from vins_core.utils.iter import all_chains
from vins_core.utils.misc import (
    parallel,
    ParallelItemError,
    call_once_on_dict,
    multiply_dicts,
    best_score_dicts,
    match_with_dict_of_patterns,
    copy_subtree,
)
from vins_core.utils.logging import log_call, lazy_logging


@pytest.mark.parametrize('tz, expected_tz', [
    ('Asia/Hanoi', 'Asia/Bangkok'),
    ('Europe/Warsaw', 'Europe/Warsaw'),
    ('GMT+03:00', 'Etc/GMT+3'),
    ('GMT+08:00', 'Etc/GMT+8'),
    ('Asia/Khabarovsk', 'Asia/Vladivostok'),
    ('Asia/Beijing', 'Asia/Shanghai'),
    ('Asia/AbuDhabi', 'Asia/Dubai'),
    ('Asia/Severo-Kurilsk', 'Etc/GMT+11'),
    ('GMT+05:30', 'Asia/Kolkata'),
    ('Etc/GMT+14', 'Etc/GMT-14'),
    ('Etc/GMT+14:00', 'Etc/GMT-14'),
    ('Africa/Pretoria', 'Africa/Johannesburg'),
    ('UTC+0', 'UTC'),
    ('UTC+04', 'Etc/GMT+4'),
    ('UTC+05', 'Etc/GMT+5'),
    pytest.param('Asia/Unknown', None, marks=pytest.mark.xfail(reason='Unknown timezone not in the fixlist')),
])
def test_parse_tz(tz, expected_tz):
    assert parse_tz(tz).zone == expected_tz


def test_logging_successful_call(caplog):
    with caplog.at_level(logging.INFO):
        with log_call('test'):
            print 'test'
        assert 'Operation "test" has succeeded' in caplog.text


def test_logging_failed_call(caplog):
    with caplog.at_level(logging.INFO):
        with pytest.raises(ValueError):
            with log_call('test'):
                raise ValueError
        assert 'Operation "test" has failed' in caplog.text


def test_env_setting():
    assert get_setting('A', 'a') == 'a'
    with pytest.raises(ValueError) as excinfo:
        get_setting('B')
    assert excinfo.value.message == 'No value for "B"'
    assert get_setting('C', 'not c', yenv={'testing': 'c'}) == 'c'
    with pytest.raises(ValueError) as excinfo:
        get_setting('D', yenv=['123'])
    assert excinfo.value.message == '"yenv" parameter must be dict-object'

    os.environ['VINS_TEST_ENV_SETTING'] = '1'
    assert get_setting('TEST_ENV_SETTING', yenv={'testing': '0'}) == '1'


def test_get_bool_setting():
    var_name = 'BOOL_TEST_SETTING'
    env_var_name = DEFAULT_VINS_PREFIX + var_name

    assert not get_bool_setting(var_name)

    os.environ[env_var_name] = '1'
    assert get_bool_setting(var_name)
    del os.environ[env_var_name]

    assert not get_bool_setting(var_name, default=0)

    assert get_bool_setting(var_name, default=1)

    os.environ[env_var_name] = 'no'
    assert not get_bool_setting(var_name)
    del os.environ[env_var_name]

    with pytest.raises(ValueError):
        get_bool_setting(var_name, default=0.3)

    assert get_bool_setting(var_name, default=True)

    os.environ[env_var_name] = 'xxx'
    with pytest.raises(ValueError):
        get_bool_setting(var_name)
    del os.environ[env_var_name]


def test_parallel():
    n = 100
    items = range(n)

    def power(x, exponent, **kwargs):
        return math.pow(x, exponent)

    result = parallel(power, items, num_procs=2, function_kwargs={'exponent': -1}, raise_on_error=False)
    result = [item for item in result if not isinstance(item, ParallelItemError)]
    x = np.arange(2, n, dtype='float')
    assert np.allclose(np.diff(result), 1 / (x * (1 - x)))


@pytest.mark.parametrize('raise_on_error, ignore_exceptions, filter_errors', [
    pytest.param(True, (), False, marks=pytest.mark.xfail(raises=ValueError)),
    pytest.param(True, (), True, marks=pytest.mark.xfail(raises=ValueError)),
    (True, ('ValueError',), False),
    (True, ('ValueError',), True),
    (False, ('ValueError',), False),
    (False, ('ValueError',), True)
])
def test_parallel_error(raise_on_error, ignore_exceptions, filter_errors):
    n = 100
    items = range(n)

    def power(x, exponent, **kwargs):
        return math.pow(x, exponent)

    result = parallel(
        power, items, num_procs=2, function_kwargs={'exponent': -1},
        raise_on_error=raise_on_error, filter_errors=filter_errors,
        ignore_exceptions=ignore_exceptions
    )
    assert len(result) + int(filter_errors) == len(items)
    if not filter_errors:
        assert isinstance(result[0], ParallelItemError)
        assert attr.asdict(result[0]) == {
            'item': 0,
            'message': 'math domain error',
            'exception_type': 'ValueError'
        }


def test_parallel_with_initializer():
    n = 100
    items = range(n)

    def bias(a, b):
        return 10 * a + b

    def power(x, initializer, exponent):
        bias = initializer
        return bias + math.pow(x, exponent)

    result = parallel(power, items, num_procs=2, function_kwargs={'exponent': -1},
                      initializer=bias, initializer_kwargs={'a': 1, 'b': 2}, raise_on_error=False)
    result = [item for item in result if not isinstance(item, ParallelItemError)]
    x = np.arange(1, n, dtype='float')
    assert np.allclose(result, 12 + 1 / x)


def test_call_once_on_dict():

    class AddCallCounter:
        def __init__(self):
            self.counter = 0

        def __call__(self, items):
            self.counter += 1
            return map(lambda x: x + self.counter, items)

    assert call_once_on_dict(AddCallCounter(), {
        'a': [0, 1, 2],
        'b': [3, 4]
    }) == {
        'a': [1, 2, 3],
        'b': [4, 5]
    }


def test_call_once_on_dict_filter_errors():

    class ErrorWhenGteFive:

        def __init__(self):
            pass

        def __call__(self, items):
            return [i if i < 5 else ParallelItemError() for i in items]

    assert call_once_on_dict(ErrorWhenGteFive(), {
        'a': [1, 3, 5],
        'b': [6, 4]
    }, filter_errors=True) == {
        'a': [1, 3],
        'b': [4]
    }


@pytest.mark.parametrize("data, keys, result", [
    ({}, 'a', {}),
    ({}, 'a.b', {}),
    ({'a': 1}, 'a', {}),
    ({'b': 1}, 'a', {'b': 1}),
    ({'a': {'b': 1}}, 'b', {'a': {'b': 1}}),
    ({'a': {'b': 1}}, 'a', {}),
    ({'a': {'b': 1}}, 'a.b', {'a': {}}),
    ({'a': {'b': 1}}, ['a.b', 'c'], {'a': {}}),
    ({'a': {'b': 1}}, ['a.b', 'a'], {}),
    ({'a': ['b']}, 'a', {}),
    ({'a': ['b']}, 'a.b', {'a': ['b']}),
])
def test_delete_keys_from_dict(data, keys, result):
    if isinstance(keys, basestring):
        keys = [keys]

    assert delete_keys_from_dict(data, keys) == result


@pytest.mark.parametrize("dicts, result", [
    ([{'a': 1, 'b': 2}, {'a': 2, 'b': 3}], {'a': 2, 'b': 6}),
    ([defaultdict(lambda: 0, {'a': 1, 'b': 2}), defaultdict(lambda: 1, {'b': 2, 'c': 3})], {'a': 1, 'b': 4, 'c': 0})
])
def test_multiply_dicts(dicts, result):
    assert multiply_dicts(dicts) == result


@pytest.mark.parametrize("dicts, result", [
    ([{'a': 1, 'b': 2}, {'a': 2, 'b': 3}], {'a': 2, 'b': 3}),
    ([defaultdict(lambda: 0, {'a': 1, 'b': 2}), defaultdict(lambda: 1, {'b': 5, 'c': 3})], {'a': 1, 'b': 5, 'c': 3})
])
def test_best_score_dicts(dicts, result):
    assert best_score_dicts(dicts) == result


def test_all_chains():
    reference = [['aa', 'ba'], ['aa', 'bb'], ['ab', 'ba'], ['ab', 'bb'], ['ac', 'ba'], ['ac', 'bb']]
    chains = list(all_chains([['aa', 'ab', 'ac'], ['ba', 'bb']]))

    assert len(chains) == len(reference)

    for ref, chain in zip(reference, chains):
        assert ref == chain


@pytest.mark.parametrize("pattern, inspected_object, try_regex, result", [
    ("aa.*", "aardvark", False, False),
    ("aa.*", "aardvark", True, True),
    (100500, "100500", True, False),
    ("100500", 100500, True, False),
    (100500, 100500, True, True),
    ({"a": 1}, "a", True, False),
    ("a", {"a": 1}, True, False),
    ({"a": 1}, {"a": 1}, True, True),
    ({"a": 1}, {"a": 1, "b": 2}, True, True),
    ({"a": 1, "b": 0}, {"a": 1, "b": 2}, True, False),
    ({"a": 1, "b": 2}, {"a": 1, "b": 2}, True, True),
    ({"a": {"b": {"c": "d"}}}, {"a": {"b": {"c": "d"}}}, True, True),
    ({"a": {"b": {}}}, {"a": {"b": {"c": None}}}, True, True),
])
def test_match_with_dict_of_patterns(pattern, inspected_object, try_regex, result):
    assert match_with_dict_of_patterns(pattern, inspected_object, try_regex) == result


@pytest.mark.parametrize('source, paths, expected_result', [
    ({'a': {'b': {'c': 'd', 'e': 'f'}, 'g': {'h': 'i'}}, 'j': 'k'}, [['a', 'b', 'e'], ['j']],
     {'a': {'b': {'e': 'f'}}, 'j': 'k'}),
    ('{"a": 1, "b": {"c": 2}}', [['b', 'c']], {'b': {'c': 2}}),
    pytest.param('{"a": 1, "b": {"c": 2}}', [['a', 'd']], {'a': {'b': {'d': 2}}}, marks=pytest.mark.xfail(reason='Invalid path')),
])
def test_copy_subtree(source, paths, expected_result):
    assert copy_subtree(source, paths) == expected_result


def test_vins_temp_dir_exists(tmpdir, mocker):
    mocker.patch('vins_core.utils.data.tempfile.gettempdir', return_value=str(tmpdir))

    vins_temp_dir()             # create at first time

    mocker.patch('vins_core.utils.data.os.path.exists', return_value=False)

    vins_temp_dir()             # no exception


def test_lazy_logging():
    call_count = [0, 0]

    @lazy_logging
    def log_format1(v):
        call_count[0] += 1
        return str(v) + '_format1'

    @lazy_logging
    def log_format2(v):
        call_count[1] += 1
        return str(v) + '_format2'

    log = log_format1('val1')
    assert call_count == [0, 0]
    assert str(log) == 'val1_format1'
    assert call_count == [1, 0]
    assert str(log) == 'val1_format1'
    assert call_count == [1, 0]
    assert unicode(log) == u'val1_format1'
    assert call_count == [1, 0]
    log = log_format1('val2')
    assert call_count == [1, 0]
    assert str(log) == 'val2_format1'
    assert call_count == [2, 0]
    assert str(log) == 'val2_format1'
    assert call_count == [2, 0]
    assert unicode(log) == u'val2_format1'
    assert call_count == [2, 0]

    log = log_format2('val1')
    assert call_count == [2, 0]
    assert str(log) == 'val1_format2'
    assert call_count == [2, 1]
    assert str(log) == 'val1_format2'
    assert call_count == [2, 1]
    assert unicode(log) == u'val1_format2'
    assert call_count == [2, 1]
    log = log_format2('val2')
    assert call_count == [2, 1]
    assert str(log) == 'val2_format2'
    assert call_count == [2, 2]
    assert str(log) == 'val2_format2'
    assert call_count == [2, 2]
    assert unicode(log) == u'val2_format2'
    assert call_count == [2, 2]
