import json
import os
import logging
from contextlib import contextmanager

import alice.tests.library.vault as vault
import ydb


_ydb_config = None
_ydb_database = '/ru/alice/prod/mocker'


def _make_default_ydb_config():
    global _ydb_config
    if not _ydb_config:
        _ydb_config = ydb.DriverConfig(
            endpoint='ydb-ru.yandex.net:2135',
            database=_ydb_database,
            auth_token=vault.get_oauth_token('ydb_robot-bassist'),
        )
    return _ydb_config


stats_table_name = 'evo_test_stats'
opuses_table_name = 'evo_tests/opuses'
sensors_table_name = 'evo_tests/solomon_sensors'
marker_tests_table_name = 'evo_tests/marker_tests'


class DefaultEncoder(json.JSONEncoder):
    def default(self, o):
        if hasattr(o, 'dict'):
            return o.dict()
        return o.__dict__


class EvoOpusRow(object):
    columns = {
        'timestamp': 'Uint64',
        'text': 'Utf8',
        'labels': 'Utf8',
        'opus': 'String',
        'meta': 'Json',
    }
    __slots__ = columns.keys()

    def __init__(self, timestamp, text, opus, labels, meta):
        self.timestamp = int(timestamp.timestamp())
        self.text = text
        self.opus = opus
        self.labels = labels
        self.meta = json.dumps(meta or {})


class EvoSolomonSensorRow(object):
    columns = {
        'timestamp': 'Uint64',
        '_key': 'Utf8',
        'sensor': 'Utf8',
        'surface': 'Utf8',
        'component': 'Utf8',
        'filename': 'Utf8',
        'class_name': 'Utf8',
        'test_name': 'Utf8',
        'value': 'Uint64',
    }
    __slots__ = columns.keys()

    def __init__(self, timestamp, _key, sensor, surface, component, filename, class_name, test_name, value):
        self.timestamp = int(timestamp)
        self._key = _key
        self.sensor = sensor
        self.surface = surface
        self.component = component
        self.filename = filename
        self.class_name = class_name
        self.test_name = test_name
        self.value = value


class EvoStatsRow(object):
    columns = {
        'timestamp': 'Uint64',
        'name': 'Utf8',
        'status': 'Utf8',
        'versions': 'Json',
        'marks': 'Json',
        'extra_info': 'Json',
    }
    __slots__ = columns.keys()

    def __init__(self, timestamp, name, status, versions, marks, extra_info):
        self.timestamp = timestamp
        self.name = name
        self.status = status
        self.versions = json.dumps(versions or {}, cls=DefaultEncoder)
        self.marks = json.dumps(marks or {}, cls=DefaultEncoder)
        self.extra_info = json.dumps(extra_info or {})


@contextmanager
def connect_to_ydb(ydb_driver_config=None):
    logger = logging.getLogger('ydb.connection')
    logger.setLevel(logging.INFO)

    driver = ydb.Driver(ydb_driver_config or _make_default_ydb_config())
    try:
        driver.wait(timeout=5)
        yield driver
    except TimeoutError:
        raise RuntimeError('Connect failed to YDB')
    finally:
        driver.stop()


def select_opus(session, table_path, text, labels):
    select_opus_query = session.prepare(f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");

        DECLARE $text AS Utf8;
        DECLARE $labels AS Utf8;

        SELECT * FROM {os.path.basename(table_path)}
        WHERE text = $text AND labels = $labels;
    """)
    results = session.transaction(ydb.SerializableReadWrite()).execute(
        select_opus_query,
        {'$text': text, '$labels': labels},
        commit_tx=True,
    )
    return results[0].rows[-1] if results[0].rows else None


def upsert_into(session, table_path, row_desc, values):
    prepared_query = session.prepare(f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");

        DECLARE $values AS "List<Struct<
            {','.join([f'{name}:{type}' for name, type in row_desc.columns.items()])}
        >>";

        REPLACE INTO {os.path.basename(table_path)}
        SELECT {', '.join(row_desc.__slots__)}
        FROM AS_TABLE($values)
    """)
    session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query, commit_tx=True, parameters={'$values': values}
    )


def replace_into(session, table_path, values):
    prepared_query = session.prepare(f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");

        DECLARE $values AS "List<Struct<
            {','.join([f'{name}:{type}' for name, type in EvoStatsRow.columns.items()])}
        >>";

        REPLACE INTO {os.path.basename(table_path)}
        SELECT
            CAST(timestamp AS DateTime) AS datetime, {', '.join(EvoStatsRow.__slots__)}
        FROM AS_TABLE($values)
    """)
    session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query, commit_tx=True, parameters={'$values': values}
    )


def select_marker_tests(session, table_path):
    select_opus_query = session.prepare(f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");

        DECLARE $text AS Utf8;
        DECLARE $labels AS Utf8;

        SELECT * FROM {os.path.basename(table_path)}
    """)
    results = session.transaction(ydb.SerializableReadWrite()).execute(
        select_opus_query,
        commit_tx=True,
    )
    return results[0].rows if results[0].rows else None


def download_parametrize():
    table_path = os.path.join(_ydb_database, marker_tests_table_name)
    with connect_to_ydb() as ydb_driver:
        session = ydb_driver.table_client.session().create()
        return select_marker_tests(session, table_path)


def save(values):
    table_path = os.path.join(_ydb_database, stats_table_name)
    with connect_to_ydb() as ydb_driver:
        session = ydb_driver.table_client.session().create()
        replace_into(session, table_path, values)


def download_opus(text, labels=None):
    table_path = os.path.join(_ydb_database, opuses_table_name)
    with connect_to_ydb() as ydb_driver:
        session = ydb_driver.table_client.session().create()
        result = select_opus(session, table_path, text, labels)
        return result.opus if result else None


def upload_opus(values):
    table_path = os.path.join(_ydb_database, opuses_table_name)
    with connect_to_ydb() as ydb_driver:
        session = ydb_driver.table_client.session().create()
        upsert_into(session, table_path, EvoOpusRow, values)


def upload_solomon_sensors(values):
    table_path = os.path.join(_ydb_database, sensors_table_name)
    with connect_to_ydb() as ydb_driver:
        session = ydb_driver.table_client.session().create()
        upsert_into(session, table_path, EvoSolomonSensorRow, values)
