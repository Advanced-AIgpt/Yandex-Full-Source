import argparse
import json
import os
from datetime import datetime, timedelta

import ydb


def is_table_exists(driver, path):
    try:
        return driver.scheme_client.describe_path(path).is_table()
    except ydb.SchemeError:
        return False


def upsert_simple(session, table_path, values):
    upsert_into = f"""
        PRAGMA TablePathPrefix("{os.path.dirname(table_path)}");
        UPSERT INTO {os.path.basename(table_path)}
            (datetime, timestamp, name, status, versions, login, is_release)
        VALUES (
            CAST({values.timestamp} AS DateTime),
            {values.timestamp},
            "{values.name}",
            "{values.status}",
            "{values.versions}",
            "{values.login}",
            {values.is_release}
        );
    """
    print(upsert_into)
    session.transaction().execute(upsert_into, commit_tx=True)


class Row(object):
    __slots__ = ('timestamp', 'name', 'status', 'versions', 'login', 'is_release')

    def __init__(self, timestamp, name, status, versions, login, is_release):
        self.timestamp = int(timestamp.timestamp())
        self.name = name
        self.status = status
        self.versions = json.dumps(versions or {})
        self.login = login
        self.is_release = is_release


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
    print(replace_into)
    prepared_query = session.prepare(replace_into)
    session.transaction(ydb.SerializableReadWrite()).execute(
        prepared_query, commit_tx=True, parameters={'$values': values}
    )


def create_stats(session, table_path):
    session.create_table(
        table_path,
        ydb.TableDescription()
        .with_column(ydb.Column('timestamp', ydb.OptionalType(ydb.DataType.Uint64)))
        .with_column(ydb.Column('datetime', ydb.OptionalType(ydb.DataType.Datetime)))
        .with_column(ydb.Column('name', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('status', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('versions', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('marks', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('extra_info', ydb.OptionalType(ydb.DataType.Json)))
        .with_primary_keys('name', 'timestamp')
    )


def create_opuses(session, table_path):
    session.create_table(
        table_path,
        ydb.TableDescription()
        .with_column(ydb.Column('timestamp', ydb.OptionalType(ydb.DataType.Uint64)))
        .with_column(ydb.Column('text', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('opus', ydb.OptionalType(ydb.DataType.String)))
        .with_column(ydb.Column('labels', ydb.OptionalType(ydb.ListType(ydb.DataType.Utf8))))
        .with_column(ydb.Column('meta', ydb.OptionalType(ydb.DataType.Json)))
        .with_primary_keys('text', 'labels', 'timestamp')
    )


def create_marker_tests(session, table_path):
    session.create_table(
        table_path,
        ydb.TableDescription()
        .with_column(ydb.Column('app_preset', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('cards', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('client_time', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('device_state', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('directives', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('experiments', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('location', ydb.OptionalType(ydb.DataType.Json)))
        .with_column(ydb.Column('scenario', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('product_scenario', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('real_reqid', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('text', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('timezone', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('intent', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('opus', ydb.OptionalType(ydb.DataType.String)))
        .with_primary_keys('real_reqid')
    )


def create_errors(session, table_path):
    session.create_table(
        table_path,
        ydb.TableDescription()
        .with_column(ydb.Column('timestamp', ydb.OptionalType(ydb.DataType.Uint64)))
        .with_column(ydb.Column('_key', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('sensor', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('surface', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('component', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('filename', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('class_name', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('test_name', ydb.OptionalType(ydb.DataType.Utf8)))
        .with_column(ydb.Column('value', ydb.OptionalType(ydb.DataType.Uint64)))
        .with_primary_keys('_key', 'timestamp')
    )


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-e', '--endpoint', default='ydb-ru.yandex.net:2135', help='Endpoint url to use')
    parser.add_argument('-d', '--database', default='/ru/alice/prod/mocker', help='Name of the database to use')
    parser.add_argument('-p', '--path', default='evo_tests/marker_tests', help='')
    parser.add_argument('-a', '--auth', default=None, help='')
    parser.add_argument('-f', '--force', action='store_true', help='')

    return parser.parse_args()


def main():
    args = parse_args()

    if args.auth:
        driver_config = ydb.DriverConfig(
            args.endpoint, args.database, auth_token=args.auth,
        )
    else:
        driver_config = ydb.DriverConfig(
            args.endpoint,
            args.database,
            credentials=ydb.construct_credentials_from_environ(),
        )
    with ydb.Driver(driver_config) as driver:
        try:
            driver.wait(timeout=5)
        except TimeoutError:
            raise RuntimeError('Connect failed to YDB')

        session = driver.table_client.session().create()

        table_path = os.path.join(args.database, args.path)

        # if is_table_exists(driver, table_path):
        #     session.drop_table(table_path)

        create_marker_tests(session, table_path)

        result = session.describe_table(table_path)
        print(f'describe table: {table_path}')
        for column in result.columns:
            print(f'column: {column.name}, {column.type.item}')

        return
        now = datetime.now()
        columns = [
            Row(
                timestamp=now,
                name='test_name',
                status='OK',
                versions={'megamind': 83},
                login='me',
                is_release=False,
            ),
            Row(
                timestamp=now,
                name='test_name_2',
                status='OK',
                versions={'megamind': 83},
                login='me',
                is_release=True,
            ),
            Row(
                timestamp=now + timedelta(minutes=1),
                name='test_name',
                status='FAIL',
                versions={'megamind': 84},
                login='me',
                is_release=False,
            ),
        ]

        replace_into(session, table_path, columns)
