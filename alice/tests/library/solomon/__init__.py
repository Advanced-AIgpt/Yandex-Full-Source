import logging
from datetime import datetime

import alice.tests.library.vault as vault
import alice.tests.library.ydb as ydb
import requests
from library.python.monlib.encoder import create_json_encoder
from library.python.monlib.metric_registry import MetricRegistry


class _Sensor(object):
    def __init__(self, sensor, surface, component, class_name, filename, test_name):
        self.sensor = sensor
        self.surface = surface
        self.component = component
        self.class_name = class_name
        self.filename = filename
        self.test_name = test_name

    def dict(self):
        return self.__dict__

    @property
    def _key(self):
        return '_'.join(self.dict().values())

    def __hash__(self):
        return hash(self._key)

    def __eq__(self, other):
        return self._key == other._key


class _SensorFactory(object):

    types = ['total', 'failed', 'skipped', 'passed']

    def __init__(self, test):
        test_path, class_name = test.parent.nodeid.split('::', 1)
        component, filename = test_path.split('.', 1)
        self._component = component
        self._class_name = class_name
        self._filename = filename
        self._test_name = test.originalname
        self._surface = test.callspec.getparam('surface').__name__
        self._sensors = {_: self._create(_) for _ in _SensorFactory.types}

    def _create(self, sensor_type):
        return _Sensor(
            sensor=f'test.{sensor_type}_count',
            surface=self._surface,
            component=self._component,
            class_name=self._class_name,
            filename=self._filename,
            test_name=self._test_name,
        )

    def __iter__(self):
        return iter(self._sensors.values())

    @property
    def total(self):
        return self._sensors['total']

    def get(self, sensor_type):
        return self._sensors[sensor_type]


def send_sensors(cluster, service, timestamp, test_data):
    metrics = {}
    for item in test_data:
        sensors = _SensorFactory(item)
        if sensors.total not in metrics:
            for sensor in sensors:
                metrics[sensor] = 0
        metrics[sensors.total] += 1
        metrics[sensors.get(item.test_result.outcome)] += 1

    registry = MetricRegistry()
    for sensor, value in metrics.items():
        registry.int_gauge(sensor.dict()).set(value)

    encoder = create_json_encoder(None, 2)
    registry.accept(datetime.utcfromtimestamp(timestamp), encoder)
    encoder.close()

    oauth_token = vault.get_oauth_token('solomon_robot-bassist')
    response = requests.post(
        f'https://solomon.yandex-team.ru/api/v2/push?project=alice_evo&cluster={cluster}&service={service}',
        data=encoder.dumps(),
        headers={
            'Content-Type': 'application/json',
            'Authorization': f'OAuth {oauth_token}',
        },
    )
    logging.info(f'Metrics were send to solomon: {response.text}')

    ydb.upload_solomon_sensors([
        ydb.EvoSolomonSensorRow(
            timestamp=timestamp,
            value=value,
            _key=sensor._key,
            **sensor.dict()
        ) for sensor, value in metrics.items()
    ])
    logging.info('Metrics were send to ydb')
