import os
import time

import alice.uniproxy.library.backends_common.ydb as uniydb

from alice.uniproxy.library.messenger.client_entry_base import ClientEntryBase
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config, environment
from alice.uniproxy.library.utils.hostname import current_hostname

from ydb.table import StaleReadOnly

MSSNGR_READ_TIMEOUT = config['messenger']['locator'].get('ydb', {}).get('read_timeout', 1.5)
MSSNGR_WRITE_TIMEOUT = config['messenger']['locator'].get('ydb', {}).get('write_timeout', 1.5)
MSSNGR_REMOVE_TIMEOUT = config['messenger']['locator'].get('ydb', {}).get('remove_timeout', 2)
MSSNGR_YDB_POOL_SIZE = config['messenger']['locator'].get('ydb', {}).get('pool_size', 100)
MSSNGR_YDB_RM_POOL_SIZE = config['messenger']['locator'].get('ydb', {}).get('remove_pool_size', 10)
SHARDS_PER_REQUEST = 16
if os.environ.get('YDB_SHARD_PER_REQUEST'):
    SHARDS_PER_REQUEST = int(os.environ.get('YDB_SHARD_PER_REQUEST'))
TABLE_SHARD_NUM = 0x100 / 128

CLIENT_LOCATOR_GETS_RETRIES = config['messenger']['locator']['retries']

STORE_QUERY = '''
        DECLARE $guid AS String;
        DECLARE $host AS String;
        DECLARE $ts AS Uint64;

        UPSERT INTO [{table_name}]
        (guid, host, ts)
        VALUES
        ($guid, $host, $ts);
    '''

REMOVE_QUERY = '''
        DECLARE $host AS String;
        DECLARE $ts AS Uint64;
        DECLARE $guid AS String;

        DELETE FROM [{table_name}]
            WHERE host = $host AND
                ts < $ts AND
                guid = $guid;
    '''

LIST_QUERY = '''
        DECLARE $guids AS "List<String>";

        SELECT guid, host, ts
            FROM [{table_name}]
            WHERE guid IN $guids;
    '''

_storage = None


def guids_by_requests(guids):
    '''guids must be sorted!
    '''
    shards_count = 0
    prev_shard_id = -1
    ret = []
    guid_list = []
    for g in guids:
        try:
            h = int(g[:2], 16)
        except:
            Logger.get('.locator').info("can't contvert guid '{}'".format(g))
            continue

        shard_id = h // TABLE_SHARD_NUM
        if prev_shard_id != shard_id:
            prev_shard_id = shard_id
            shards_count += 1

        if shards_count > SHARDS_PER_REQUEST:
            ret.append(guid_list)
            shards_count = 1
            guid_list = []

        guid_list.append(g)

    if guid_list:
        ret.append(guid_list)

    return ret


# for reusing code in delivery
class ClientEntry(ClientEntryBase):
    def __init__(self):
        self._data = []

    def __len__(self):
        return len(self._data)

    def add(self, guid, hostname, ts):
        self._data.append((guid, hostname, ts))

    def update(self, entries):
        self._data.extend(entries._data)

    def enumerate_locations(self, **kwargs):
        ret = []
        for loc in self._data:
            HostNo, _, _, _ = self.encoded_hostname(loc[1], default='')
            ret.append((loc[0], loc[1], HostNo))
        return ret


class Storage:
    def __init__(self, driver, table_name, init_func=None):
        Logger.get('.locator').info('connect to table {}'.format(table_name))
        self.sessions = uniydb.make_session_pool(driver, MSSNGR_YDB_POOL_SIZE, min_pool_size=MSSNGR_YDB_POOL_SIZE, initializer=init_func)
        self.r_settings = uniydb.get_settings(MSSNGR_READ_TIMEOUT)
        self.w_settings = uniydb.get_settings(MSSNGR_WRITE_TIMEOUT)

        self.rm_sessions = uniydb.make_session_pool(driver, MSSNGR_YDB_RM_POOL_SIZE, min_pool_size=MSSNGR_YDB_RM_POOL_SIZE)
        self.rm_settings = uniydb.get_settings(MSSNGR_REMOVE_TIMEOUT)

        self.table_name = table_name


async def get_executor(init=None, is_remover=False):
    global _storage
    if not _storage:
        ydb_config = config['messenger']['locator']['ydb']
        ydb_token = os.environ.get('YDB_TOKEN_MSSNGR')
        if not ydb_token and environment == 'development':
            raise Exception('YDB_TOKEN_MSSNGR must be set')

        driver = await uniydb.do_connect_ydb(ydb_config, ydb_token, 'mssngr_delivery')
        if init:
            _storage = Storage(driver, ydb_config['table_name'], init(ydb_config['table_name']))
        else:
            _storage = Storage(driver, ydb_config['table_name'])

    if is_remover:
        return uniydb.YdbExecutor(_storage.rm_sessions, 'mssngr_location')

    return uniydb.YdbExecutor(_storage.sessions, 'mssngr_location')


class YdbClientLocator:
    REMOVE = 'remove'
    STORE = 'store'
    LIST = 'list'

    @staticmethod
    async def initialize():
        """ This func must be executed only on start up because of long timeout
        """
        prepare_timeout = 100

        def _f(table_name):
            def _init(session):
                # ATTENTION! this op is synchronous, but it's ok because in kikimr it will be done in speacial thread
                queries = [q.format(table_name=table_name) for q in [STORE_QUERY, REMOVE_QUERY, LIST_QUERY]]
                try:
                    for q in queries:
                        session.prepare(q, uniydb.get_settings(prepare_timeout))
                except Exception as e:
                    logger = Logger.get('mssngr.locator.initialize')
                    logger.exception(e)
                    logger.error("can't prepare requests")
            return _init

        await get_executor(_f)
        global _storage
        while _storage.sessions.busy_size != 0:
            time.sleep(1)

    @staticmethod
    def init_counters():
        uniydb.register_counters_for(
            'mssngr_location',
            [
                YdbClientLocator.REMOVE,
                YdbClientLocator.STORE,
                YdbClientLocator.LIST,
            ]
        )

    @staticmethod
    def make_timestamp():
        return int(time.time() * 1000000)

    async def store(guid):
        executor = await get_executor()
        new_ts = YdbClientLocator.make_timestamp()

        # build store query parameters
        parameters = {
            '$guid': guid.encode('utf-8'),
            '$host': current_hostname().encode('utf-8'),
            '$ts': new_ts,
        }

        async def _store(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, STORE_QUERY, _storage.table_name, _storage.w_settings)
            return await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.w_settings))

        retries_count = 0
        while retries_count < CLIENT_LOCATOR_GETS_RETRIES:
            try:
                await executor.execute(_store, YdbClientLocator.STORE, MSSNGR_WRITE_TIMEOUT)
                return True
            except Exception as exc:
                Logger.get('mssngr.locator.store').exception(exc)
            finally:
                retries_count += 1
        return False

    async def list(guids):
        """ return all records for current group_id
        """
        executor = await get_executor()

        parameters = {
            '$guids': [g.encode('utf-8') for g in guids],
        }

        Logger.get().debug('{} ydb: try get list of {}'.format(id(parameters), len(guids)))

        async def _list(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, LIST_QUERY, _storage.table_name, _storage.r_settings)
            return await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.r_settings))

        retries_count = 0
        result_sets = None
        while result_sets is None and retries_count < CLIENT_LOCATOR_GETS_RETRIES:
            try:
                result_sets = await executor.execute(_list, YdbClientLocator.LIST, MSSNGR_READ_TIMEOUT, tx_mode=StaleReadOnly())
            except Exception as exc:
                Logger.get('mssngr.locator.list').exception(exc)
            finally:
                retries_count += 1

        res = ClientEntry()
        if not result_sets:
            return res

        rows = result_sets[0].rows
        for r in rows:
            res.add(r['guid'].decode(), r['host'].decode(), r['ts'])

        Logger.get().debug('{} ydb: got: {}'.format(id(parameters), len(res._data)))
        return res

    async def remove(guid, host, ts):
        executor = await get_executor(is_remover=True)

        parameters = {
            '$guid': guid.encode('utf-8'),
            '$host': host.encode('utf-8'),
            '$ts': ts,
        }

        async def _remove(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, REMOVE_QUERY, _storage.table_name, _storage.rm_settings)
            await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.rm_settings))

        retries_count = 0
        while retries_count < CLIENT_LOCATOR_GETS_RETRIES:
            try:
                await executor.execute(_remove, YdbClientLocator.REMOVE, MSSNGR_REMOVE_TIMEOUT)
                return True
            except Exception:
                # Logger.get('mssngr.locator.remove').exception(exc)
                pass
            finally:
                retries_count += 1
        return False


class ClientLocator:
    def __init__(self):
        self.client = YdbClientLocator

    # ----------------------------------------------------------------------------------------------------------------
    def update_location(self, guid, client_id):
        ret = []
        ret.append(self.client.store(guid))
        return ret

    # ----------------------------------------------------------------------------------------------------------------
    def remove_location(self, guid, client_id, hostname=None, timestamp=None):
        ret = [
            self.client.remove(guid, hostname, timestamp)
        ]

        return ret

    # ----------------------------------------------------------------------------------------------------------------
    def resolve_locations(self, guids):
        return self.client.list(guids)
