import concurrent
import hashlib
import json
import os
from functools import partial

import tornado.ioloop
import ydb

import alice.uniproxy.library.backends_common.ydb as uniydb

from alice.uniproxy.library.backends_common.protohelpers import proto_from_json, proto_to_json
from alice.uniproxy.library.backends_bio.yabio_storage import get_key, get_all_keys
from alice.uniproxy.library.backends_bio.yabio_storage import YabioStorage, YabioStorageAccessor
from alice.uniproxy.library.backends_bio.yabio_storage import ContextStorage, LIST_QUERY, connect_ydb
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger

from voicetech.library.proto_api.yabio_pb2 import YabioContext


def main():
    import argparse

    test_group_id = 'test_group_id'
    test_dev_model = 'test_dev_model'
    test_dev_manuf = 'test_dev_manuf'

    GlobalCounter.init()
    Logger.init('yabio_storage', True)
    logger = Logger.get('yabio_storage')
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--cmd',
        help='create-table|backup|convert-users|drop-table|dump-context|grant-permissions|save-all|save|load|test-delete',
        required=True,
    )
    parser.add_argument('--directory', required=False, default=None)
    parser.add_argument('--user', required=False, default=None)
    parser.add_argument('--group-id', required=False, default=test_group_id)
    parser.add_argument('--dev-model', required=False, default=test_dev_model)
    parser.add_argument('--dev-manuf', required=False, default=test_dev_manuf)
    parser.add_argument('--table-name', required=False, default='/ru/alice/test/vins-context/yabio_context_restyled')
    parser.add_argument('--updated-at', required=False, default=None)
    parser.add_argument('--file', required=False, default=None)
    args = parser.parse_args()

    FAIL_CODE = 1

    try:
        driver = tornado.ioloop.IOLoop.current().run_sync(connect_ydb)
    except (TimeoutError, concurrent.futures.TimeoutError, tornado.gen.TimeoutError) as exc:
        Logger.get().error('fail on create driver=TimeoutError')
        return FAIL_CODE
    except Exception as exc:
        logger.exception('fail on create driver')
        return FAIL_CODE

    session = driver.table_client.session().create()
    if args.cmd == 'create-table':
        columns = {
            'shard_key': ydb.DataType.Uint64,
            'group_id': ydb.DataType.String,
            'dev_model': ydb.DataType.String,
            'dev_manuf': ydb.DataType.String,
            'context': ydb.DataType.String,
            'version': ydb.DataType.Uint32,
            'updated_at': ydb.DataType.Uint64,
        }

        def table_description():
            td = ydb.TableDescription()
            for name, _type in columns.items():
                td = td.with_column(ydb.Column(name, ydb.OptionalType(_type)))
            return td.with_primary_keys('shard_key', 'group_id', 'dev_model', 'dev_manuf')

        session.create_table(args.table_name, table_description())
        logger.info('table [{}] created'.format(args.table_name))
    elif args.cmd == 'backup':
        table_from = args.table_name
        table_to = table_from + '_backup'
        session.copy_table(table_from, table_to)
    elif args.cmd == 'drop-table':
        session.drop_table(args.table_name)
        logger.info('table [{}] dropped'.format(args.table_name))
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
    elif args.cmd == 'save':
        ctx = None
        with open(args.file) as f:
            data = json.load(f)
            ctx = proto_from_json(YabioContext, data)

        accessor = YabioStorage(driver, args.table_name).create_accessor(
            group_id=args.group_id,
            dev_model=args.dev_model,
            dev_manuf=args.dev_manuf,
        )
        tornado.ioloop.IOLoop.current().run_sync(partial(accessor.save, ctx.SerializeToString()))
        logger.info('successfully save record to [{}]'.format(args.table_name))

    elif args.cmd == 'save-all':
        data = None
        with open(args.file) as f:
            data = json.load(f)

        for u in data:
            ctx = proto_from_json(YabioContext, u['context'])
            accessor = YabioStorage(driver, args.table_name, pool_size=1).create_accessor(
                group_id=u['group_id'],
                dev_model=u['dev_model'],
                dev_manuf=u['dev_manuf'],
            )
            tornado.ioloop.IOLoop.current().run_sync(partial(accessor.save, ctx.SerializeToString()))
            logger.info('successfully saved record for [{} | {} | {}] to [{}]'.format(
                u['group_id'],
                u['dev_model'],
                u['dev_manuf'],
                args.table_name)
            )
        logger.info('End')

    elif args.cmd == 'load':
        accessor = YabioStorage(driver, args.table_name, pool_size=1).create_accessor(
            group_id=args.group_id,
            dev_model=args.dev_model,
            dev_manuf=args.dev_manuf,
        )
        recs = tornado.ioloop.IOLoop.current().run_sync(accessor.load)
        logger.info('load test record from [{}]: {}'.format(args.table_name, len(recs)))
        json_data = []
        for rec in recs:
            context = YabioContext()
            context.ParseFromString(rec['context'])
            rec['context'] = proto_to_json(context)
            rec['group_id'] = args.group_id
            rec['dev_model'] = args.dev_model
            rec['dev_manuf'] = args.dev_manuf
            json_data.append(rec)
        with open(args.file, 'w') as f:
            json.dump(json_data, f, indent=2)
    elif args.cmd == 'test-delete' or args.cmd == 'delete':
        accessor = YabioStorage(driver, args.table_name).create_accessor(
            group_id=args.group_id, dev_model=args.dev_model, dev_manuf=args.dev_manuf)
        result = tornado.ioloop.IOLoop.current().run_sync(partial(accessor.delete))
        logger.info('delete test record key={} from [{}]: {}'.format(
            get_key(args.group_id, args.dev_model, args.dev_manuf), args.table_name, result))
    elif args.cmd == 'dump-context':
        ctx_storage = ContextStorage(
            '',
            group_id=args.group_id,
            dev_model=args.dev_model,
            dev_manuf=args.dev_manuf,
            table_name=args.table_name,
        )
        recs = tornado.ioloop.IOLoop.current().run_sync(ctx_storage.load)
        logger.info('load test record from [{}]: {}'.format(args.table_name, recs))
        ctx_storage.log_hr()
    elif args.cmd == 'list-group-ids':
        if args.updated_at is None:
            print('--updated-at must be present! This is timestamp')
            return FAIL_CODE

        table_to = args.table_name
        updated_at = args.updated_at
        database_to = table_to[:table_to.rindex('/')]
        logger.info('load group ids from {}'.format(table_to))
        ydb_token_from = os.getenv('YDB_TOKEN_FROM')
        assert ydb_token_from is not None, 'import require YDB_TOKEN_FROM env. var.'
        driver_to = tornado.ioloop.IOLoop.current().run_sync(
            partial(connect_ydb, ydb_token=ydb_token_from))

        sessions_pool_to = uniydb.make_session_pool(driver_to)
        active_session = None
        active_session_to = None
        keys = []

        offset = 0
        ended = False

        def _execute_transaction(session, offset):
            q = tornado.ioloop.IOLoop.current().run_sync(partial(uniydb.prepare_query_for_table, session,
                                                                 table_to, LIST_QUERY))
            with session.transaction() as tx:
                parameters = {'$upd': int(updated_at), '$off': offset}
                result_sets = tornado.ioloop.IOLoop.current().run_sync(partial(tx.async_execute, q, parameters, commit_tx=True))
                ended = (len(result_sets[0].rows) == 0)
                for row in result_sets[0].rows:
                    group_id = row['group_id'].decode()
                    dev_model = row['dev_model'].decode()
                    dev_manuf = row['dev_manuf'].decode()
                    ctx_storage = ContextStorage(
                        '',
                        group_id=group_id,
                        dev_model=dev_model,
                        dev_manuf=dev_manuf,
                        table_name=args.table_name,
                    )
                    recs = tornado.ioloop.IOLoop.current().run_sync(ctx_storage.load)
                    logger.info('load test record from [{}]: {}'.format(args.table_name, recs))
                    if len(ctx_storage.yabio_context.users) != 0:
                        keys.append(get_key(group_id, dev_model, dev_manuf))
                    offset += 1
                    if offset % 100 == 0:
                        logger.info(offset)
                    return ended, offset

        while not ended:
            ended, offset = sessions_pool_to.retry_operation_sync(_execute_transaction, None, offset)

        logger.info('End')

    elif args.cmd == 'migrate-station-to-yandexstation':
        driver = tornado.ioloop.IOLoop.current().run_sync(partial(connect_ydb))
        sessions_pool = uniydb.make_session_pool(driver)

        rows = []

        with sessions_pool.checkout(blocking=True) as session:
            table_iter = session.read_table(
                args.table_name,
                columns=('group_id', 'dev_model', 'dev_manuf')
            )

            while True:
                try:
                    rs = next(table_iter)
                    for row in rs.rows:
                        rows.append((row['group_id'].decode(), row['dev_model'].decode(), row['dev_manuf'].decode()))

                except StopIteration:
                    break

        for gid, dev_model, dev_manuf in rows:
            if dev_model != 'station':
                continue
            ctx_storage = ContextStorage(
                '',
                group_id=gid,
                dev_model=dev_model,
                dev_manuf=dev_manuf,
                table_name=args.table_name,
            )
            tornado.ioloop.IOLoop.current().run_sync(ctx_storage.load)

            ctx_storage.dev_model = 'yandexstation'
            ctx_storage.key = get_key(gid, ctx_storage.dev_model, dev_manuf)
            ctx_storage.ydb_accessor._dev_model = 'yandexstation'.encode('utf-8')

            key = get_key(gid, dev_model, dev_manuf)
            ctx_storage.ydb_accessor._shard_key = ctx_storage.ydb_accessor._compute_shard_key(key)

            tornado.ioloop.IOLoop.current().run_sync(ctx_storage.save)

            print('saved {}'.format(ctx_storage.key))

        print('done')

    elif args.cmd == 'convert-users':
        # json file with new data for db.
        file_from = args.file
        if not file_from:
            Logger.get().warning('arg --file must be present!')
            return FAIL_CODE

        table_to = args.table_name
        database_to = table_to[:table_to.rindex('/')]
        if 'prod' in table_to:
            question = "You try to change PROD DB. Are you sure? Input full 'yes' or something else if no: "
            if input(question).lower() != 'yes':
                logger.info("you choose 'no'")
                return FAIL_CODE

        logger.info('import user records from file {} to DB {}'.format(file_from, database_to))
        logger.info('import user records to {}'.format(table_to))
        ydb_token_from = os.getenv('YDB_TOKEN_FROM')
        assert ydb_token_from is not None, 'import require YDB_TOKEN_FROM env. var. (& optionally YDB_TOKEN_TO)'
        ydb_token_to = os.getenv('YDB_TOKEN_TO')
        if ydb_token_to is None:
            ydb_token_to = ydb_token_from
        driver_to = tornado.ioloop.IOLoop.current().run_sync(
            partial(connect_ydb, ydb_token=ydb_token_to))

        sessions_pool_to = uniydb.make_session_pool(driver_to)
        active_session = None
        active_session_to = None
        keys_from = None
        full_data = None
        # Get all Keys
        with open(file_from) as f:
            full_data = json.load(f)
            keys_from = set([get_key(**r) for r in full_data])

        # check data in file
        for d in full_data:
            if not d['requestId'] or \
               not d['source'] or \
               not d['compatibilityTag'] or \
               not d['format'] or \
               not d['voiceprint'] or \
               not d['user_id'] or \
               not d['group_id'] or \
               not d['dev_model'] or \
               not d['dev_manuf'] or \
               not (d['mds_key'] and d['mds_key'].startswith('http://')):

                Logger.get().warning('incomplete data in file')
                return FAIL_CODE

        keys_to = get_all_keys(sessions_pool_to, table_to)

        logger.info('loaded... {} keys'.format(len(keys)))
        updated = 0
        deleted = 0
        not_touched = 0
        total = 0
        for key in keys_to:
            ctx_storage = ContextStorage(
                '',
                group_id=key['group_id'],
                dev_model=key['dev_model'],
                dev_manuf=key['dev_manuf'],
                table_name=args.table_name,
            )
            recs = tornado.ioloop.IOLoop.current().run_sync(ctx_storage.load)
            if len(ctx_storage.yabio_context.users) == 0:
                continue

            total += 1

            vp = {}
            for data in full_data:
                if get_key(data) == key:
                    voiceprint = {}
                    voiceprint['request_id'] = data['requestId']
                    voiceprint['compatibility_tag'] = data['compatibilityTag']
                    voiceprint['format'] = data['format']
                    voiceprint['source'] = data['source']
                    voiceprint['voiceprint'] = data['voiceprint']
                    voiceprint['mds_url'] = data['mds_key']

                    if data['user_id'] not in vp:
                        vp[data['user_id']] = []
                    vp[data['user_id']].append(voiceprint)

            logger.info('{} - {}'.format(key, len(vp)))
            if len(vp) > 0:
                logger.info('user in "{}" was updated'.format(key))
                updated += 1
                tornado.ioloop.IOLoop.current().run_sync(partial(ctx_storage.update_users_voiceprints, vp))

        for key in keys_from:
            if key not in keys_to:
                logger.info('user from "{}" not used'.format(key))

        logger.info('total={} updated={} deleted={} not_touched={}'.format(total, updated, deleted, not_touched))
        logger.info('End')

    elif args.cmd == 'import':
        table_from = '/ru/alice/prod/vins-context/yabio_storage'
        table_to = '/ru/alice/test/vins-context/yabio_storage_backup'
        database_from = table_from[:table_from.rindex('/')]
        database_to = table_to[:table_to.rindex('/')]
        logger.info('import records from DB {} to {}'.format(database_from, database_to))
        logger.info('import records from {} to {}'.format(table_from, table_to))
        ydb_token_from = os.getenv('YDB_TOKEN_FROM')
        assert ydb_token_from is not None, 'import require YDB_TOKEN_FROM env. var. (& optionally YDB_TOKEN_TO)'
        ydb_token_to = os.getenv('YDB_TOKEN_TO')
        if ydb_token_to is None:
            ydb_token_to = ydb_token_from
        driver_from = tornado.ioloop.IOLoop.current().run_sync(
            partial(connect_ydb, ydb_token=ydb_token_from))
        driver_to = tornado.ioloop.IOLoop.current().run_sync(
            partial(connect_ydb, ydb_token=ydb_token_to))
        query_get = '''
            DECLARE $shard_key AS Uint64;
            DECLARE $group_id AS String;
            DECLARE $dev_model AS String;
            DECLARE $dev_manuf AS String;

            SELECT context
                FROM [{table_name}]
                WHERE shard_key = $shard_key AND
                    group_id = $group_id AND
                    dev_model = $dev_model AND
                    dev_manuf = $dev_manuf;
        '''
        sessions_pool_from = uniydb.make_session_pool(driver_from)
        sessions_pool_to = uniydb.make_session_pool(driver_to)
        active_session = None

        keys = get_all_keys(sessions_pool_from, table_from)

        def key_has_users(session, q, key):
            group_id, dev_model, dev_manuf = key
            tmp_key = get_key(group_id, dev_model, dev_manuf)
            parameters = {
                '$group_id': group_id,
                '$dev_model': dev_model,
                '$dev_manuf': dev_manuf,
                '$shard_key': int(hashlib.md5(tmp_key).hexdigest(), 16) & ((1 << 64) - 1),
            }
            with session.transaction() as tx:
                result_sets = tx.execute(q, parameters, commit_tx=True)
                rows = result_sets[0].rows
                if not rows:
                    return None
                ctx = YabioContext()
                ctx.ParseFromString(rows[0]['context'])
                if len(ctx.users) != 0:
                    return rows[0]['context']

        total_ctx = 0
        old_has_user = 0
        both_has_user = 0
        need_move = 0
        with sessions_pool_from.acquire() as active_session, sessions_pool_to.acquire() as active_session_to:
            for key in keys:
                total_ctx += 1
                with active_session.transaction(), active_session_to.transaction():
                    q = tornado.ioloop.IOLoop.current().run_sync(partial(
                        uniydb.prepare_query_for_table, active_session, query_get, table_from))
                    q2 = tornado.ioloop.IOLoop.current().run_sync(partial(
                        uniydb.prepare_query_for_table, active_session_to, query_get, table_to))
                    ctx_from = key_has_users(active_session, q, key)
                    if ctx_from is not None:
                        old_has_user += 1
                        ctx_to = key_has_users(active_session_to, q2, key)
                        if ctx_to is not None:
                            both_has_user += 1
                            logger.debug('both base has users for key={}'.format(key))
                        else:
                            need_move += 1
                            logger.debug('need move context for key={}'.format(key))
                            tornado.ioloop.IOLoop.current().run_sync(partial(
                                YabioStorageAccessor(
                                    sessions_pool_to,
                                    group_id=key[0],
                                    dev_model=key[1],
                                    dev_manuf=key[2]
                                ).save,
                                ctx_from)
                            )

        logger.info('total_ctx={} has_old_user={} both_has_user={} need_move={}'.format(
            total_ctx, old_has_user, both_has_user, need_move
        ))

    elif args.cmd == 'help':
        logger.info('test help :)')
    else:
        logger.exception('unexpected command [{0}]'.format(args.cmd))
        return FAIL_CODE


if __name__ == '__main__':
    ret_code = main()
    exit(0 if ret_code is None else ret_code)
