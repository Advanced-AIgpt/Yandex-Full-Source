# coding: utf-8

import mock
import pytest

from nlu_service.services import unistat


def test_counter_value_defaults_to_0():
    # arrange
    c = unistat._Counter()

    # assert
    assert c.value == 0


def test_counter_increment_defaults_to_1():
    # arrange
    c = unistat._Counter()

    # act
    c.incr()

    # assert
    assert c.value == 1


def test_counter_increment_non_default():
    # arrange
    c = unistat._Counter()

    # act
    c.incr(value=10)

    # assert
    assert c.value == 10


def test_hgram_throws_on_empty_bounds():
    # assert
    with pytest.raises(ValueError):
        unistat._Histogram(bounds=[])


@mock.patch('nlu_service.services.unistat.logger', autospec=True)
def test_hgram_warns_on_value_lower_than_left_bound(mocked_logger):
    # arrange
    hgram = unistat._Histogram(bounds=[0])

    # act
    hgram.write_value(-1)

    # assert
    assert len(mocked_logger.method_calls) == 1
    assert mocked_logger.method_calls[0][0] == 'warn'


def test_hgram_includes_left_bound():
    # arrange
    hgram = unistat._Histogram(bounds=[0., 1.0])

    # act
    hgram.write_value(0.)

    # assert
    assert hgram.buckets[0][1] == 1
    assert hgram.buckets[1][1] == 0


def test_hgram_doesnt_include_right_bound():
    # arrange
    hgram = unistat._Histogram(bounds=[0., 1.0])

    # act
    hgram.write_value(1.)

    # assert
    assert hgram.buckets[0][1] == 0
    assert hgram.buckets[1][1] == 1


def test_hgram_puts_large_values_to_right_bucket():
    # arrange
    hgram = unistat._Histogram(bounds=[0., 1.0, 2.0])

    # act
    hgram.write_value(1000.)

    # assert
    assert hgram.buckets[0][1] == 0
    assert hgram.buckets[1][1] == 0
    assert hgram.buckets[2][1] == 1


def test_all_hgrams_are_present_in_registry():
    """Проверка, что при добавлении новой гистограммы она была добавлена в _histogram_registry"""
    # assert
    for hgram in unistat.HistogramName:
        assert hgram.value in unistat._histogram_registry


def test_all_counters_are_present_in_registry():
    """Проверка, что при добавлении нового счетчика он был добавлен в _counter_registry"""
    # assert
    for counter in unistat.CounterName:
        assert counter.value in unistat._counter_registry


def test_unistat_returns_only_zeros_on_start():
    # act
    metrics = unistat.get_serialized_values()

    # assert
    for name, metric in metrics:
        assert name in unistat._counter_registry or unistat._histogram_registry

        if name in unistat._counter_registry:
            assert metric == 0
        elif name in unistat._histogram_registry:
            for left_boundary, weight in metric:
                assert weight == 0


def test_write_hgram_value_throws_on_unknown_metric():
    with pytest.raises(AttributeError):
        unistat.write_hgram_value('xxx', 1)


def test_incr_counter_throws_on_unknown_metric():
    with pytest.raises(AttributeError):
        unistat.incr_counter('xxx')
