import pytest
from fakeredis import FakeStrictRedis

from alice.paskills.penguinarium.util.metrics import (
    Metric, HistogramAggregator, RateAggregator, CounterAggregator,
    SensorsConfig, DummyMetricsStorage, RedisMetricsStorage, SensorsRegistry,
    sensors
)


@pytest.mark.parametrize(
    'metric',
    [
        Metric('m1', 1, {'l1': 'v1', 'l2': 'v2'}),
        Metric('m2', 1.5, {})
    ]
)
def test_metric_serialize(metric):
    assert Metric.from_bytes(*metric.to_bytes()) == metric


def test_hist_agg():
    agg = HistogramAggregator(bins=[1, 10, 30, 50])

    name = 'name'
    labels = {'l': 'v'}
    state = agg.update_state(
        state=[],
        metrics=[
            Metric('m1', 10, {'l1': 'v1', 'l2': 'v2'}, 20.),
            Metric('m2', 15., {}, 100.),
            Metric('m2', 55., {}, 300.)
        ],
        name=name,
        labels=labels
    )
    assert state == [
        Metric(name, 0, {'bin': '1', **labels}, 0.),
        Metric(name, 0, {'bin': '10', **labels}, 0.),
        Metric(name, 2, {'bin': '30', **labels}, 100.),
        Metric(name, 0, {'bin': '50', **labels}, 0.),
        Metric(name, 1, {'bin': 'inf', **labels}, 300.),
    ]


def test_rate_agg():
    agg = RateAggregator()

    name = 'name'
    labels = {'l': 'v'}
    state = agg.update_state(
        state=[Metric('m1', 10, {'l1': 'v1', 'l2': 'v2'}, 5.)],
        metrics=[
            Metric('m1', 10, {'l1': 'v1', 'l2': 'v2'}, 20.),
            Metric('m2', 15, {}, 1.),
            Metric('m2', 55, {}, 3.)
        ],
        name=name,
        labels=labels
    )

    assert state == [Metric(name, 90, labels, 0)]


def test_counter_agg():
    agg = CounterAggregator()

    name = 'name'
    labels = {'l': 'v'}
    state = agg.update_state(
        state=[Metric('m1', 10, {'l1': 'v1', 'l2': 'v2'}, 5.)],
        metrics=[
            Metric('m1', 10, {'l1': 'v1', 'l2': 'v2'}, 20.),
            Metric('m2', 15, {}, 1.),
            Metric('m2', 55, {}, 3.)
        ],
        name=name,
        labels=labels
    )

    assert state == [Metric(name, 90, labels, 20.)]


def test_sensors_config():
    bins = [1, 10, 30, 50]
    sc = SensorsConfig({
        'http_requests': {'type': 'rate'},
        'http_response_time': {'type': 'hist', 'bins': bins},
    })

    assert type(sc.get_aggregator('http_requests')) is RateAggregator
    assert type(sc.get_aggregator('http_response_time')) is HistogramAggregator
    assert sc._conf['http_response_time'].bins == bins
    assert sc.get_aggregator('not_really_agg') is None

    assert sc.get_kind('http_requests') == 'RATE'
    assert sc.get_kind('http_response_time') == 'IGAUGE'
    assert sc.get_kind('not_really_agg') == 'DGAUGE'


@pytest.mark.asyncio
async def test_dummy_storage():
    dms = DummyMetricsStorage()
    assert len([s async for s in dms.get_state()]) == 0


class MockedFakeStrictRedis(FakeStrictRedis):
    async def lrange(self, *args, **kwargs):
        return super().lrange(*args, **kwargs)

    async def iscan(self, *args, **kwargs):
        _, keys = super().scan(*args, **kwargs)
        for k in keys:
            yield k

    def pipeline(self):
        return self

    async def execute(self):
        pass


@pytest.mark.asyncio
async def test_redis_storage():
    bins = [1, 10, 30, 50]
    rms = RedisMetricsStorage(
        redis_client=MockedFakeStrictRedis(),
        config={
            'http_requests': {'type': 'rate'},
            'http_response_time': {'type': 'hist', 'bins': bins},
        },
        cache_max_size=1
    )

    await rms.add(Metric('http_requests', 1, {}))
    await rms.add(Metric('http_requests', 1, {}))
    await rms.add(Metric('http_response_time', 5, {}))

    state = [s async for s in rms.get_state()]
    assert len(state) == len(bins) + 1 + 1


def test_repr():
    sr = SensorsRegistry()
    assert sr.__repr__() == 'SensorRegistry(n=0)'

    sr._registry = {1: 1, 2: 2, 3: 3}
    assert sr.__repr__() == 'SensorRegistry(n=3)'


@sensors.with_labels_context({'the_key': 'the_value'})
async def foo():
    assert {'the_key': 'the_value'} in sensors._stack


@pytest.mark.asyncio
async def test_with_labels_context():
    assert {'the_key': 'the_value'} not in sensors._stack
    await foo()
    assert {'the_key': 'the_value'} not in sensors._stack


def test_checkers():
    with pytest.raises(ValueError):
        Metric('', 1., {})

    with pytest.raises(TypeError):
        Metric('a', 1., [])

    with pytest.raises(ValueError):
        Metric('a', 1., {i: i for i in range(13)})

    with pytest.raises(ValueError):
        Metric('a', 1., {'': 'value'})

    with pytest.raises(ValueError):
        Metric('a', 1., {'key': ''})
