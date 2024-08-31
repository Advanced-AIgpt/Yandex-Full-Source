import tornado.ioloop
import ydb

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
import alice.uniproxy.library.backends_common.ydb as uniydb


_context_storage_config = config['vins']['context_storage']


async def connect_ydb():
    return await uniydb.do_connect_ydb(_context_storage_config)


def get_table_name():
    return _context_storage_config['table_name']


def main():
    import argparse

    Logger.init('vins-context-storage', True)
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmd', help='create-table|drop-table|grant-permissions', required=True)
    parser.add_argument('--directory', required=False, default=None)
    parser.add_argument('--user', required=False, default=None)
    parser.add_argument('--context-length', required=False, default=None,
                        help='min length of context to delete', type=int)
    parser.add_argument('--table-name', required=False, default=None, help='table full path')
    parser.add_argument('--key', required=False, default=None)
    parser.add_argument('--shard-key', required=False, default=None, type=int)
    args = parser.parse_args()

    driver = tornado.ioloop.IOLoop.current().run_sync(connect_ydb)
    session = driver.table_client.session().create()
    if args.cmd == 'create-table':
        session.create_table(
            get_table_name(),
            ydb.TableDescription()
            .with_column(ydb.Column('shard_key', ydb.OptionalType(ydb.DataType.Uint64)))
            .with_column(ydb.Column('key', ydb.OptionalType(ydb.DataType.String)))
            .with_column(ydb.Column('data', ydb.OptionalType(ydb.DataType.String)))
            .with_column(ydb.Column('updated_at', ydb.OptionalType(ydb.DataType.Uint64)))
            .with_primary_keys('shard_key', 'key'))
        Logger.get('.').info('table created')
    elif args.cmd == 'drop-table':
        session.drop_table(get_table_name())
        Logger.get('.').info('table dropped')
    elif args.cmd == 'grant-permissions':
        driver.scheme_client.modify_permissions(
            args.directory,
            ydb.ModifyPermissionsSettings()
            .grant_permissions(
                args.user + '@staff', (
                    'ydb.generic.read',
                    'ydb.generic.write',
                )
            )
        )
    elif args.cmd == 'delete-session':

        sessions_pool = uniydb.make_session_pool(driver)

        query = '''
            DECLARE $shard_key AS Uint64;
            DECLARE $key AS String;

            DELETE FROM [{table_name}]
                WHERE shard_key = $shard_key
                  AND key = $key;
        '''.format(table_name=args.table_name)

        lines = []
        with open('keys', 'r') as f:
            lines = f.readlines()

        cnt = 0
        for line in lines:
            q = line.split()

            parameters = {
                '$shard_key': int(q[1]),
                '$key': q[0].encode('utf-8'),
            }
            with sessions_pool.checkout(blocking=True) as session:
                session.prepare(query)
                with session.transaction() as tx:
                    tx.execute(query, parameters=parameters, commit_tx=True)
                    cnt += 1
            print(cnt)

    else:
        raise RuntimeError('unexpected command [{0}]'.format(args.cmd))


if __name__ == '__main__':
    main()
