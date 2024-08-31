from typing import (
    Union, Dict, Any, Tuple, List, AsyncGenerator,
    ContextManager, AsyncContextManager, Callable
)
from abc import ABC, abstractmethod
from bisect import bisect
import re
import time
import copy
import logging
import asyncio
from threading import RLock
from contextvars import ContextVar
from contextlib import asynccontextmanager, contextmanager
from functools import wraps
from weakref import proxy
from struct import pack, unpack
import attr
import aioredis

logger = logging.getLogger(__name__)
_sensor_value_regexp = re.compile(r'^[a-zA-Z0-9./@_][ 0-9a-zA-Z./@_,:;()\[\]<>-]{0,198}$')
Labels = Dict[str, str]


def check_labels(obj: Any, attr: str, value: Labels) -> None:
    if len(value) > 12:
        raise ValueError(f'Too many labels {value}')

    for label, val in value.items():
        if not re.match(r'^[a-zA-Z][0-9a-zA-Z_]{0,31}$', label):
            raise ValueError(f'Invalid label name "{label}"')
        if not _sensor_value_regexp.match(val):
            raise ValueError(f'Invalid value "{val}" for label "{label}"')


def check_name(obj: Any, attr: str, value: str) -> None:
    if not _sensor_value_regexp.match(value):
        raise ValueError(f'Invalid name "{value}" for object "{obj}"')


@attr.s(frozen=True, slots=True)
class Metric:
    name: str = attr.ib(validator=check_name)
    value: Union[int, float] = attr.ib()
    labels: Labels = attr.ib(validator=[
        attr.validators.instance_of(dict),
        check_labels
    ])
    update_time: float = attr.ib(default=attr.Factory(time.time))

    @staticmethod
    def _pack_pairs(pairs: Tuple[str, str]) -> bytes:
        res = b''

        for one, two in pairs:
            one = one.encode('ascii')
            two = two.encode('ascii')
            res += pack('<BB', len(one), len(two)) + one + two

        return res

    @staticmethod
    def _unpack_pairs(bts: bytes) -> List[Tuple[str]]:
        res = []
        start = 0
        offset = 0

        while offset < len(bts):
            offset += 2  # two bytes
            (size1, size2) = unpack('<BB', bts[start:offset])
            one = bts[offset: offset + size1].decode('ascii')

            offset += size1
            two = bts[offset: offset + size2].decode('ascii')
            offset += size2
            start = offset
            res.append((one, two))

        return res

    def to_bytes(self) -> Tuple[bytes]:
        """
        Format:

        Key:
        1 byte  - value is float (bool)
        2 byte  - name's length N (unsigned short)
        N bytes - name (ascii)

        rest: pairs of labels
          1 byte  - key's length K (unsigned char)
          1 byte  - value's length V (unsigned char)
          K bytes - key (ascii)
          V bytes - value (ascii)
          ...

        Value:
        8 bytes – update_time (double)
        8 bytes - value (long or float)
        """

        # pack key
        value_is_float = isinstance(self.value, float)
        res_key = pack('<?', value_is_float)

        name = self.name.encode('ascii')
        res_key += pack('<H', len(name))
        res_key += name
        res_key += self._pack_pairs(sorted(self.labels.items()))

        # pack value
        assert self.update_time is not None and self.value is not None, \
               'Empty Metric is not serializable'

        res_value = pack('<d', self.update_time)

        if value_is_float:
            res_value += pack('<d', self.value)
        else:
            res_value += pack('<q', self.value)

        return (res_key, res_value)

    @classmethod
    def from_bytes(cls, key_bytes: bytes, value_bytes: bytes = None):
        offset = 3
        value_is_float, name_len = unpack('<?H', key_bytes[:offset])

        name = key_bytes[offset: offset + name_len].decode('ascii')
        offset += name_len

        labels = cls._unpack_pairs(key_bytes[offset:])

        if value_bytes is None:
            # empty metric
            update_time = value = None
        elif value_is_float:
            update_time, value = unpack('<dd', value_bytes)
        else:
            update_time, value = unpack('<dq', value_bytes)

        return cls(name, value, dict(labels), update_time)


class Aggregator(ABC):
    @abstractmethod
    def update_state(
        self,
        state: List[Metric],
        metrics: List[Metric],
        name: str,
        labels: Labels
    ) -> List[Metric]:
        pass


@attr.s
class HistogramAggregator(Aggregator):
    kind: str = 'IGAUGE'
    bins: int = attr.ib()

    def update_state(
        self,
        state: List[Metric],
        metrics: List[Metric],
        name: str,
        labels: Labels
    ) -> List[Metric]:
        n_bins = len(self.bins) + 1
        hist = [0] * n_bins
        times = [0.] * n_bins

        for metric in metrics:
            bin = bisect(self.bins, metric.value)
            hist[bin] += 1
            times[bin] = max(times[bin], metric.update_time)

        bins = list(map(str, self.bins))
        bins.append('inf')

        state = []
        for bin, value, ts in zip(bins, hist, times):
            labels = dict(labels, bin=str(bin))
            state.append(Metric(name, value, labels, ts))

        return state


class GaugeAggregator(Aggregator):
    def update_state(
        self,
        state: List[Metric],
        metrics: List[Metric],
        name: str,
        labels: Labels
    ) -> List[Metric]:
        return copy.copy(metrics)


class IGaugeAggregator(GaugeAggregator):
    kind: str = 'IGAUGE'


class DGaugeAggregator(GaugeAggregator):
    kind: str = 'DGAUGE'


class RateAggregator(Aggregator):
    kind: str = 'RATE'

    def update_state(
        self,
        state: List[Metric],
        metrics: List[Metric],
        name: str,
        labels: Labels
    ) -> List[Metric]:
        value = sum(metric.value for metric in metrics)
        value += 0 if not state else state[0].value
        return [Metric(name, value, labels, 0)]


class CounterAggregator(Aggregator):
    kind = 'COUNTER'

    def update_state(
        self,
        state: List[Metric],
        metrics: List[Metric],
        name: str,
        labels: Labels
    ) -> List[Metric]:
        value = 0
        max_time = 0.

        for metric in metrics:
            value += metric.value
            max_time = max(max_time, metric.update_time)

        value += 0 if not state else state[0].value
        return [Metric(name, value, labels, max_time)]


def convert_labels(labels: Labels) -> Labels:
    return {key: str(value) for key, value in labels.items()}


@attr.s
class Sensor:
    name: str = attr.ib(validator=check_name)
    labels: Labels = attr.ib(validator=[
        attr.validators.instance_of(dict),
        check_labels
    ])
    _storage: List[Metric] = attr.ib()

    def update_labels(self, labels: Labels) -> None:
        self.labels.update(labels)

    async def new(
        self,
        value: Union[int, float],
        labels: Labels
    ) -> None:
        lbls = convert_labels(dict(self.labels, **(labels or {})))
        await self._storage.add(Metric(self.name, value, lbls))

    def copy(self):
        return self.__class__(
            self.name,
            copy.deepcopy(self.labels),
            self._storage
        )


class SensorsConfig(object):
    def __init__(self, conf: Dict[str, Dict]):
        res = {}
        cls_map = {
            'igauge': IGaugeAggregator,
            'dgauge': DGaugeAggregator,
            'counter': CounterAggregator,
            'rate': RateAggregator,
            'hist': HistogramAggregator,
        }
        for key, value in conf.items():
            conf = value.copy()
            cls = cls_map[conf.pop('type')]
            res[key] = cls(**conf)

        self._conf = res

    def get_aggregator(self, sensor_name: str) -> Aggregator:
        if sensor_name not in self._conf:
            logger.warning('Unknown metric for %s sensor' % sensor_name)
            return None

        return self._conf[sensor_name]

    def get_kind(self, sensor_name: str) -> str:
        if sensor_name not in self._conf:
            return 'DGAUGE'

        return self._conf[sensor_name].kind


class MetricsStorage(ABC):
    @abstractmethod
    async def add(self, sensor: Metric) -> None:
        pass

    @abstractmethod
    async def flush(self) -> None:
        pass

    @abstractmethod
    async def get_state(
        self,
        sensor_name: str = None
    ) -> AsyncGenerator[Metric, None]:
        pass


class DummyMetricsStorage(MetricsStorage):
    async def add(self, sensor: Metric) -> None:
        logger.debug('Got sensor %s' % repr(sensor))

    async def flush(self) -> None:
        pass

    async def get_state(
        self,
        sensor_name: str = None
    ) -> AsyncGenerator[Metric, None]:
        return
        yield


class RedisMetricsStorage(MetricsStorage):
    def __init__(
        self,
        redis_client,
        config: Dict[str, Dict],
        cache_max_size: int = 1000
    ):
        self.conf = SensorsConfig(config)
        self._cache_max_size = cache_max_size
        self._data = []
        self._lock = RLock()
        self._redis = redis_client
        self._sensors_key = b'sensors'
        self._state_key = b'state'

    async def add(self, sensor: Metric) -> None:
        with self._lock:
            self._data.append(sensor)

        if len(self._data) > self._cache_max_size:
            await self.flush()

    async def flush(self) -> None:
        with self._lock:
            async with asyncio.Lock():
                data = self._data
                pipe = self._redis.pipeline()

                for sensor in data:
                    key, value = sensor.to_bytes()
                    sensor_key = self._sensors_key + b':' + key
                    pipe.rpush(sensor_key, value)

                try:
                    await pipe.execute()
                    self._data = []
                except Exception as exc:
                    logger.exception("Can't flush sensors to Redis: %s", exc)

    async def _get_cur_state(self, key: bytes) -> List[Metric]:
        return [Metric.from_bytes(v[16:], v[:16])
                for v in await self._redis.lrange(key, 0, -1)]

    def _set_cur_state(
        self,
        pipe: aioredis.commands.Pipeline,
        state_key: bytes,
        state: List[Metric]
    ) -> None:
        pipe.delete(state_key)
        for metric in state:
            key, value = metric.to_bytes()
            pipe.rpush(state_key, value + key)

    async def _gen_keys(
        self,
        sensor_prefix: str,
        state_prefix: str
    ) -> AsyncGenerator[bytes, None]:
        async for key in self._redis.iscan(match=sensor_prefix + b'*'):
            yield key[len(sensor_prefix):]

        async for key in self._redis.iscan(match=state_prefix + b'*'):
            yield key[len(state_prefix):]

    async def get_state(
        self,
        sensor_name: str = None
    ) -> AsyncGenerator[Metric, None]:
        await self.flush()

        sensor_prefix = self._sensors_key + b':'
        state_prefix = self._state_key + b':'
        keys = [k async for k in self._gen_keys(
            sensor_prefix=sensor_prefix,
            state_prefix=state_prefix
        )]

        with self._lock:
            async with asyncio.Lock():
                for key in set(keys):
                    # get metrics
                    sensor_key = sensor_prefix + key
                    state_key = state_prefix + key
                    ref = Metric.from_bytes(key)
                    if sensor_name is not None and ref.name != sensor_name:
                        continue

                    metrics = [Metric.from_bytes(key, v)
                               for v in await self._redis.lrange(sensor_key, 0, -1)]
                    cur_state = await self._get_cur_state(state_key)

                    # calc new state
                    aggregator = self.conf.get_aggregator(ref.name)
                    if aggregator is None:
                        continue

                    new_state = aggregator.update_state(
                        cur_state, metrics, ref.name, ref.labels)

                    for m in new_state:
                        yield m

                    # update state in redis
                    pipe = self._redis.pipeline()
                    pipe.ltrim(sensor_key, len(metrics), -1)
                    self._set_cur_state(pipe, state_key, new_state)
                    await pipe.execute()


stack_var = ContextVar('stack_var', default=None)


class SensorsRegistry:
    def __init__(self):
        self._stack_var = stack_var
        self._registry = {}
        self.storage = DummyMetricsStorage()
        self._lock = RLock()

    def setup(self, storage: MetricsStorage) -> None:
        with self._lock:
            self.storage = storage

    def _get_sensor(self, name: str) -> Sensor:
        with self._lock:
            return self._registry.get(name)

    def set_empty_labels_stack(self) -> None:
        self._stack_var.set([])

    @property
    def _stack(self) -> List[Labels]:
        if self._stack_var.get() is None:
            self._stack_var.set([])

        return self._stack_var.get()

    def _register_sensor(self, name: str, labels: Labels) -> Sensor:
        item = Sensor(name, labels or {}, proxy(self.storage))
        with self._lock:
            self._registry[name] = item
        return item

    def new_sensor(self, name: str, labels: Labels = None) -> Sensor:
        """ Create new or get from registry sensor with specific name """
        sensor = self._get_sensor(name)
        if sensor is None:
            sensor = self._register_sensor(name, labels)

        sensor = sensor.copy()

        for labels in self._stack:
            sensor.update_labels(labels)

        return sensor

    @contextmanager
    def labels_context(self, labels: Labels) -> ContextManager:
        self._stack.append(labels)
        try:
            yield
        finally:
            self._stack.pop()

    def with_labels_context(self, labels: Labels) -> Callable:
        def decorator(f):
            @wraps(f)
            async def inner(*args, **kwargs):
                with self.labels_context(labels):
                    return await f(*args, **kwargs)
            return inner
        return decorator

    @asynccontextmanager
    async def timer(self, name: str, labels: Labels = None) -> AsyncContextManager:
        sensor = self.new_sensor(name)
        start = time.time()
        try:
            yield
        finally:
            diff = (time.time() - start) * 1000  # milliseconds
            await sensor.new(diff, labels)

    def with_timer(self, name: str, labels: Labels = None) -> Callable:
        def decorator(f):
            @wraps(f)
            async def inner(*args, **kwargs):
                async with self.timer(name, labels):
                    return await f(*args, **kwargs)
            return inner
        return decorator

    async def set_sensor(
        self,
        name: str,
        value: Union[int, float],
        labels: Labels = None
    ) -> None:
        sensor = self.new_sensor(name)
        await sensor.new(value, labels)

    async def inc_counter(
        self,
        name: str,
        value: int = 1,
        labels: Labels = None
    ) -> None:
        await self.set_sensor(name, value, labels)

    def __repr__(self) -> str:
        return 'SensorRegistry(n=%s)' % len(self._registry)

    async def get_sensors(self) -> AsyncGenerator[Dict, None]:
        async for metric in self.storage.get_state():
            kind = self.storage.conf.get_kind(metric.name)
            res = {
                'labels': dict(metric.labels, sensor=metric.name),
                'value': metric.value,
                'kind': kind,
            }
            ts = int(metric.update_time)
            if ts:
                res['ts'] = ts

            yield res


sensors = SensorsRegistry()
