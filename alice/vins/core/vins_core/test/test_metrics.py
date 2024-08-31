# coding: utf-8

from random import gauss

import redis.client
import fakeredis
import pytest

from vins_core.utils.metrics import (
    SensorsRegistry,
    InMemoryMerticsStorage,
    RedisMetricsStorage,
    Metric,
)

CONF = {
    'test_gauge': {'type': 'dgauge'},
    'test_counter': {'type': 'counter'},
    'test_rate': {'type': 'rate'},
    'test_hist': {'type': 'hist', 'bins': range(10)},
}


@pytest.fixture(params=['inmemory', 'redis'])
def sensors(request):
    cls = {
        'inmemory': InMemoryMerticsStorage,
        'redis': RedisMetricsStorage,
    }[request.param]

    if request.param == 'inmemory':
        storage = cls(CONF)
    elif request.param == 'redis':
        storage = cls(fakeredis.FakeStrictRedis(), CONF)
    else:
        raise ValueError('Unknown storage %s' % request.param)

    sens = SensorsRegistry()
    sens.setup(storage)
    return sens


def test_gauge(sensors):
    for _ in range(10):
        sensors.set_sensor('test_gauge', 1)

    state = sensors.storage.get_state('test_gauge')
    assert len(state) == 10
    assert all(map(lambda x: x.value == 1, state))


def test_counter(sensors):
    for _ in range(10):
        sensors.inc_counter('test_counter')

    state = sensors.storage.get_state('test_counter')
    assert len(state) == 1
    assert state[0].value >= 10


def test_rate(sensors):
    for _ in range(100):
        sensors.inc_counter('test_rate')

    state = sensors.storage.get_state('test_rate')
    assert state == [
        Metric('test_rate', value=100, labels={}, time=0),
    ]


def test_hist(sensors):
    for _ in range(100):
        sensors.set_sensor('test_hist', gauss(5, 2))

    state = sensors.storage.get_state('test_hist')

    assert len(state) == 11
    assert sum(map(lambda x: x.value, state)) == 100
    assert map(lambda x: x.labels, state) == [{'bin': i} for i in map(str, range(10))] + [{'bin': 'inf'}]


def test_new_counter_on_second_step(sensors):
    for _ in range(10):
        sensors.inc_counter('test_counter')
    for _ in range(10):
        sensors.set_sensor('test_gauge', 1)
    for _ in range(10):
        sensors.inc_counter('test_rate')

    sensors.storage.get_state()

    for _ in range(100):
        sensors.set_sensor('test_hist', gauss(5, 2))

    state = sensors.storage.get_state('test_hist')
    assert len(state) == 11
    assert sum(map(lambda x: x.value, state)) == 100


def test_two_time_get_state(sensors):
    for _ in range(10):
        sensors.inc_counter('test_counter')
    for _ in range(10):
        sensors.set_sensor('test_gauge', 1)
    for _ in range(10):
        sensors.inc_counter('test_rate')

    sensors.storage.get_state()  # first get_state
    state = sensors.storage.get_state()  # second get_state

    assert len(state) == 0


def test_redis_exception(mocker, caplog):
    storage = RedisMetricsStorage(fakeredis.FakeStrictRedis(), CONF)
    mocker.patch.object(redis.client.Pipeline, 'execute', side_effect=Exception)
    sens = SensorsRegistry()
    sens.setup(storage)

    sens.inc_counter('test_counter')
    storage.flush()             # not raises

    assert "Can't flush sensors to Redis" in caplog.text
