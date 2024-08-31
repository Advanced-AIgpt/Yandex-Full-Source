import argparse
import os
import ydb


def create_db(driver, db):
    session = driver.table_client.session().create()

    def create_instance_name_to_id_table():
        table_name = db + '/' + 'instance_name_to_id'
        session.create_table(
            table_name,
            ydb.TableDescription()
            .with_column(ydb.Column('service_name', ydb.OptionalType(ydb.DataType.String)))
            .with_column(ydb.Column('host_name', ydb.OptionalType(ydb.DataType.String)))
            .with_column(ydb.Column('id', ydb.OptionalType(ydb.DataType.Uint64)))
            .with_primary_keys('service_name', 'host_name'))
        print('table [{0}] created'.format(table_name))

    def create_instance_id_to_name():
        table_name = db + '/' + 'instance_id_to_name'
        session.create_table(
            table_name,
            ydb.TableDescription()
            .with_column(ydb.Column('id', ydb.OptionalType(ydb.DataType.Uint64)))
            .with_column(ydb.Column('service_name', ydb.OptionalType(ydb.DataType.String)))
            .with_column(ydb.Column('host_name', ydb.OptionalType(ydb.DataType.String)))
            .with_primary_keys('id'))
        print('table [{0}] created'.format(table_name))

    create_instance_name_to_id_table()
    create_instance_id_to_name()


def drop_all_tables(driver, db):
    session = driver.table_client.session().create()
    for c in driver.scheme_client.list_directory(db).children:
        table_name = db + '/' + c.name
        session.drop_table(table_name)
        print('table [{0}] dropped'.format(table_name))


def main():
    ydb_token = os.environ.get('YDB_TOKEN')
    if not ydb_token:
        raise RuntimeError('YDB_TOKEN not defined')
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmd', help='create|recreate', required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--database', required=True)
    args = parser.parse_args()
    connection_params = ydb.ConnectionParams(args.endpoint, args.database, None, ydb_token)
    driver = ydb.Driver(connection_params)
    driver.wait()
    if args.cmd == 'create':
        create_db(driver, args.database)
    elif args.cmd == 'recreate':
        drop_all_tables(driver, args.database)
        create_db(driver, args.database)
    else:
        raise RuntimeError('unexpected command [{0}]'.format(args.cmd))


if __name__ == '__main__':
    main()
