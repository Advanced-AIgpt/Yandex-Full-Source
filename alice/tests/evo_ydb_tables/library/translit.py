import json
import os
from contextlib import contextmanager
from datetime import datetime, timedelta

import ydb



class TestStats(object):
    __slots__ = ('timestamp', 'name', 'status', 'versions', 'login', 'is_release')

    def __init__(self, timestamp, name, status, versions, login, is_release):
        self.timestamp = int(timestamp.timestamp())
        self.name = name
        self.status = status
        self.versions = json.dumps(versions or {})
        self.login = login
        self.is_release = is_release


@contextmanager
def connect(endpoint, database):
    driver = ydb.Driver(ydb.DriverConfig(
        endpoint,
        database,
        credentials=ydb.construct_credentials_from_environ(),
    ))
    try:
        driver.wait(timeout=5)
        yield driver
    except TimeoutError:
        raise RuntimeError('Connect failed to YDB')
    finally:
        driver.stop()


def replace_into(session, table_path, values):
    replace_into = f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");

        DECLARE $values AS "List<Struct<
            timestamp: Uint64,
            name: Utf8,
            status: Utf8,
            versions: Json,
            login: Utf8,
            is_release: Bool
        >>";

        REPLACE INTO {os.path.basename(table_path)}
        SELECT
            CAST(timestamp AS DateTime) AS datetime, timestamp, name, status, versions, login, is_release
        FROM AS_TABLE($values)
    """
    prepared_query = session.prepare(replace_into)
    session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query, commit_tx=True, parameters={'$values': values}
    )


def is_table_exists(driver, path):
    try:
        return driver.scheme_client.describe_path(path).is_table()
    except ydb.SchemeError:
        return False
