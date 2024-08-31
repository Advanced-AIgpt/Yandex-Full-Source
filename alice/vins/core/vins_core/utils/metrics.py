# coding: utf-8
from __future__ import absolute_import
from __future__ import division

import abc
import copy
import ujson
import logging
import re
import time
from bisect import bisect
from collections import deque, defaultdict
from functools import wraps
from threading import local, RLock
from struct import pack, unpack
from weakref import proxy

import attr
from contextlib import contextmanager


_sensor_value_regexp = re.compile(r'^[a-zA-Z0-9./@_][ 0-9a-zA-Z./@_,:;()\[\]<>-]{0,198}$')
logger = logging.getLogger(__name__)


def check_labels(obj, attr, value):
    if len(value) > 12:
        raise ValueError(
            'Too many labels %s' % value,
        )

    for label, val in value.iteritems():
        if not re.match(r'^[a-zA-Z][0-9a-zA-Z_]{0,31}$', label):
            raise ValueError('Invalid label name "%s"' % label)
        if not _sensor_value_regexp.match(val):
            raise ValueError('Invalid value "%s" for label "%s"' % (val, label))


def check_name(obj, attr, value):
    if not _sensor_value_regexp.match(value):
        raise ValueError('Invalid name "%s" for object "%s"' % (value, obj))


def convert_labels(labels):
    for key in labels:
        labels[key] = unicode(labels[key])
    return labels


class SensorsConfig(object):
    def __init__(self, conf):
        res = {}
        cls_map = {
            'igauge': IGaugeAggregator,
            'dgauge': DGaugeAggregator,
            'counter': CounterAggregator,
            'rate': RateAggregator,
            'hist': HistogramAggregator,
        }
        for key, value in conf.iteritems():
            conf = value.copy()
            cls = cls_map[conf.pop('type')]
            res[key] = cls(**conf)

        self._conf = res

    def get_aggregator(self, sensor_name):
        if sensor_name not in self._conf:
            logger.warning('Unknown metric for %s sensor' % sensor_name)
            return None
        else:
            return self._conf[sensor_name]

    def get_kind(self, sensor_name):
        if sensor_name not in self._conf:
            return 'DGAUGE'

        return self._conf[sensor_name].kind


@attr.s(frozen=True, slots=True)
class Metric(object):
    name = attr.ib(validator=check_name)
    value = attr.ib()
    labels = attr.ib(validator=[
        attr.validators.instance_of(dict),
        check_labels,
    ])
    time = attr.ib(default=attr.Factory(time.time))

    @staticmethod
    def _pack_pairs(pairs):
        res = b''

        for one, two in pairs:
            one = one.encode('ascii')
            two = two.encode('ascii')
            res += pack('<BB', len(one), len(two)) + one + two

        return res

    @staticmethod
    def _unpack_pairs(bytes):
        res = []
        start = 0
        offset = 0

        while offset < len(bytes):
            offset += 2              # two bytes
            (size1, size2) = unpack('<BB', bytes[start:offset])
            one = bytes[offset: offset + size1].decode('ascii')

            offset += size1
            two = bytes[offset: offset + size2].decode('ascii')
            offset += size2
            start = offset
            res.append((one, two))

        return res

    def to_bytes(self):
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
        8 bytes – time (double)
        8 bytes - value (long or float)
        """

        # pack key
        value_is_float = isinstance(self.value, float)
        res_key = pack('<?', value_is_float)

        name = self.name.encode('ascii')
        res_key += pack('<H', len(name))
        res_key += name
        res_key += self._pack_pairs(sorted(self.labels.iteritems()))

        # pack value
        assert self.time is None or self.value is not None, "Empty Metric is not serializable"

        res_value = pack('<d', self.time)

        if value_is_float:
            res_value += pack('<d', self.value)
        else:
            res_value += pack('<q', self.value)

        return (res_key, res_value)

    @classmethod
    def from_bytes(cls, key_bytes, value_bytes=None):
        offset = 3
        value_is_float, name_len = unpack('<?H', key_bytes[:offset])

        name = key_bytes[offset: offset + name_len].decode('ascii')
        offset += name_len

        labels = cls._unpack_pairs(key_bytes[offset:])

        if value_bytes is None:
            # empty metric
            time = value = None
        elif value_is_float:
            time, value = unpack('<dd', value_bytes)
        else:
            time, value = unpack('<dq', value_bytes)

        return cls(name, value, dict(labels), time)


class Aggregator(object):
    pass


@attr.s
class HistogramAggregator(Aggregator):
    kind = 'IGAUGE'
    bins = attr.ib()

    def update_state(self, state, metrics, name, labels):
        n_bins = len(self.bins) + 1
        hist = [0] * n_bins
        times = [0.0] * n_bins

        for metric in metrics:
            bin = bisect(self.bins, metric.value)
            hist[bin] += 1
            times[bin] = max(times[bin], metric.time)

        state = []
        bins = map(str, self.bins)
        bins.append('inf')

        for bin, value, ts in zip(bins, hist, times):
            labels = dict(labels, bin=str(bin))
            state.append(Metric(name, value, labels, ts))

        return state


class GaugeAggregator(Aggregator):
    def update_state(self, state, metrics, name, labels):
        return copy.copy(metrics)


class IGaugeAggregator(GaugeAggregator):
    kind = 'IGAUGE'


class DGaugeAggregator(GaugeAggregator):
    kind = 'DGAUGE'


class RateAggregator(Aggregator):
    kind = 'RATE'

    def update_state(self, state, metrics, name, labels):
        value = sum(metric.value for metric in metrics)
        value += 0 if not state else state[0].value
        return [Metric(name, value, labels, 0)]


class CounterAggregator(Aggregator):
    kind = 'COUNTER'

    def update_state(self, state, metrics, name, labels):
        max_time = 0.0
        value = 0

        for metric in metrics:
            value += metric.value
            max_time = max(max_time, metric.time)

        value += 0 if not state else state[0].value
        return [Metric(name, value, labels, max_time)]


class MetricsStorage(object):
    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def add(self, sensor):
        pass

    @abc.abstractmethod
    def flush(self):
        pass

    @abc.abstractmethod
    def get_state(self, sensor_name=None):
        pass


class DummyMetricsStorage(MetricsStorage):
    def add(self, sensor):
        pass

    def flush(self):
        pass

    def get_state(self, sensor_name=None):
        return []


class FileMetricsStorage(MetricsStorage):
    def __init__(self, metrics_logger):
        self._logger = metrics_logger

    def add(self, sensor):
        self._logger.debug(ujson.dumps(attr.asdict(sensor)))

    def flush(self):
        pass

    def get_state(self, sensor_name=None):
        return []


class InMemoryMerticsStorage(MetricsStorage):
    def __init__(self, config, max_size=1000):
        self.conf = SensorsConfig(config)
        self._data = defaultdict(lambda: deque(maxlen=max_size))
        self._state = {}
        self._lock = RLock()

    def add(self, sensor):
        key, value = sensor.to_bytes()
        with self._lock:
            self._data[key].append(value)

    def flush(self):
        pass

    def get_state(self, sensor_name=None):
        res = []
        with self._lock:
            flushed_sensors = []
            for key, values in self._data.iteritems():
                ref = Metric.from_bytes(key)
                if sensor_name is not None and ref.name != sensor_name:
                    continue

                metrics = [Metric.from_bytes(key, v) for v in values]

                aggregator = self.conf.get_aggregator(ref.name)
                if aggregator is None:
                    continue

                cur_state = self._state.get(key, [])
                new_state = aggregator.update_state(cur_state, metrics, ref.name, ref.labels)

                res.extend(new_state)
                self._state[key] = new_state
                self._data[key].clear()
                flushed_sensors.append(key)

            for sensor in flushed_sensors:
                del self._data[sensor]

        return res


class RedisMetricsStorage(MetricsStorage):
    def __init__(self, redis_client, config, cache_max_size=1000):
        self.conf = SensorsConfig(config)
        self._cache_max_size = cache_max_size
        self._data = []
        self._lock = RLock()
        self._redis = redis_client
        self._sensors_key = 'sensors'
        self._state_key = 'state'

    def add(self, sensor):
        with self._lock:
            self._data.append(sensor)

        if len(self._data) > self._cache_max_size:
            self.flush()

    def flush(self):
        with self._lock:
            data = self._data
            self._data = []

            pipe = self._redis.pipeline(transaction=False)

            for sensor in data:
                key, value = sensor.to_bytes()
                sensor_key = self._sensors_key + ':' + key
                pipe.rpush(sensor_key, value)

            try:
                pipe.execute()
            except Exception as exc:
                logger.exception("Can't flush sensors to Redis: %s", exc)

    def _get_cur_state(self, client, key):
        return [Metric.from_bytes(v[16:], v[:16])
                for v in client.lrange(key, 0, -1)]

    def _set_cur_state(self, client, state_key, state):
        client.delete(state_key)
        for metric in state:
            key, value = metric.to_bytes()
            client.rpush(state_key, value + key)

    def get_state(self, sensor_name=None):
        self.flush()
        sensor_prefix = self._sensors_key + ':'
        state_prefix = self._state_key + ':'
        sensors_keys = {key[len(sensor_prefix):] for key in self._redis.scan_iter(sensor_prefix + '*')}
        state = []

        with self._lock:
            for key in sensors_keys:
                # get metrics
                sensor_key = sensor_prefix + key
                state_key = state_prefix + key
                ref = Metric.from_bytes(key)
                if sensor_name is not None and ref.name != sensor_name:
                    continue

                metrics = [Metric.from_bytes(key, v)
                           for v in self._redis.lrange(sensor_key, 0, -1)]
                cur_state = self._get_cur_state(self._redis, state_key)

                # calc new state
                aggregator = self.conf.get_aggregator(ref.name)
                if aggregator is None:
                    continue

                new_state = aggregator.update_state(cur_state, metrics, ref.name, ref.labels)
                state.extend(new_state)

                # update state in redis
                pipe = self._redis.pipeline()
                # WARNING: If the result is empty list, the key will be deleted
                # This behaviour is expected
                pipe.ltrim(sensor_key, len(metrics), -1)
                self._set_cur_state(pipe, state_key, new_state)
                pipe.execute()

        return state


@attr.s
class Sensor(object):
    name = attr.ib(validator=check_name)
    labels = attr.ib(validator=[
        attr.validators.instance_of(dict), check_labels
    ])
    _storage = attr.ib()

    def update_labels(self, labels):
        self.labels.update(labels)

    def new(self, value, labels):
        lbls = convert_labels(dict(self.labels, **(labels or {})))
        self._storage.add(Metric(self.name, value, lbls))

    def copy(self):
        return self.__class__(
            self.name,
            copy.deepcopy(self.labels),
            self._storage
        )


class SensorsRegistry(object):
    def __init__(self):
        self._data = local()
        self._registry = {}
        self.storage = DummyMetricsStorage()
        self._lock = RLock()

    def setup(self, storage):
        with self._lock:
            self.storage = storage

    def _get_sensor(self, name):
        with self._lock:
            return self._registry.get(name)

    @property
    def _stack(self):
        if not hasattr(self._data, 'stack'):
            self._data.stack = []
        return self._data.stack

    def _register_sensor(self, name, labels):
        item = Sensor(name, labels or {}, proxy(self.storage))
        with self._lock:
            self._registry[name] = item
        return item

    def new_sensor(self, name, labels=None):
        """ Create new or get from registry sensor with specific name """
        sensor = self._get_sensor(name)
        if sensor is None:
            sensor = self._register_sensor(name, labels)

        sensor = sensor.copy()

        for labels in self._stack:
            sensor.update_labels(labels)

        return sensor

    @contextmanager
    def labels_context(self, labels):
        self._stack.append(labels)
        try:
            yield
        finally:
            self._stack.pop()

    def with_labels_context(self, labels):
        def decorator(f):
            @wraps(f)
            def inner(*args, **kwargs):
                with self.labels_context(labels):
                    return f(*args, **kwargs)
            return inner
        return decorator

    @contextmanager
    def timer(self, name, labels=None):
        sensor = self.new_sensor(name)
        start = time.time()
        try:
            yield
        finally:
            diff = (time.time() - start) * 1000  # milliseconds
            sensor.new(diff, labels)

    def with_timer(self, name, labels=None):
        def decorator(f):
            @wraps(f)
            def inner(*args, **kwargs):
                with self.timer(name, labels):
                    return f(*args, **kwargs)
            return inner
        return decorator

    def set_sensor(self, name, value, labels=None):
        sensor = self.new_sensor(name)
        sensor.new(value, labels)

    def inc_counter(self, name, value=1, labels=None):
        self.set_sensor(name, value, labels)

    def __repr__(self):
        return 'SensorRegistry(n=%s)' % len(self._registry)


sensors = SensorsRegistry()
