import time
import traceback
from datetime import datetime, timezone

import alice.uniproxy.library.backends_common.ydb as uniydb

from alice.uniproxy.library.protos.notificator_pb2 import TDeviceConfig
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from . import get_driver, get_ydb_config

from cityhash import hash64 as CityHash64

DEVLOC_READ_TIMEOUT = config.get('notificator', {}).get('ydb', {}).get('read_timeout', 1.5)
DEVLOC_WRITE_TIMEOUT = config.get('notificator', {}).get('ydb', {}).get('write_timeout', 3)
DEVLOC_YDB_POOL_SIZE = config.get('notificator', {}).get('ydb', {}).get('pool_size', 100)
RM_POOL_SIZE = config.get('notificator', {}).get('ydb', {}).get('rm_pool_size', 3)
CLIENT_LOCATOR_GETS_RETRIES = 2

STORE_QUERY = '''
        DECLARE $shard_key AS Uint64;
        DECLARE $puid AS String;
        DECLARE $device_id AS String;
        DECLARE $host AS String;
        DECLARE $ts AS Uint64;
        DECLARE $device_model AS String;
        DECLARE $created_at AS Datetime;
        DECLARE $config AS String;

        UPSERT INTO {table_name}
        (shard_key, puid, host, ts, device_id, device_model, ttl, config)
        VALUES
        ($shard_key, $puid, $host, $ts, $device_id, $device_model, $created_at, $config);
    '''

REMOVE_QUERY = '''
        --!syntax_v1
        DECLARE $device_id AS String;
        DECLARE $host AS String;
        DECLARE $ts AS Uint64;

        $to_delete = (
            SELECT * FROM {table_name} VIEW device_locator_list
            WHERE device_id = $device_id AND host = $host AND ts < $ts
        );

        DELETE FROM {table_name} ON
        SELECT * FROM $to_delete;
    '''

LIST_QUERY = '''
        DECLARE $shard_key AS Uint64;
        DECLARE $puid AS String;

        SELECT device_id, host, ts, device_model, config
            FROM {table_name}
            WHERE shard_key = $shard_key
              AND puid = $puid;
    '''

_storage = None

COUNTER_PREFIX = 'device_locator'


# for reusing code in delivery
class ClientEntry:
    def __init__(self):
        self._data = []

    def __len__(self):
        return len(self._data)

    def add(self, device_id, hostname, ts, device_model, config):
        self._data.append((device_id, hostname, ts, device_model, config))

    def update(self, entries):
        self._data.extend(entries._data)

    def data(self, **kwargs):
        return self._data


class Storage:
    def __init__(self, driver, table_name, init_func=None):
        Logger.get('.device.locator').info('connect to table {}'.format(table_name))
        self.sessions = uniydb.make_session_pool(driver, DEVLOC_YDB_POOL_SIZE)
        self.remove_sessions = uniydb.make_session_pool(driver, RM_POOL_SIZE)
        self.r_settings = uniydb.get_settings(DEVLOC_READ_TIMEOUT)
        self.w_settings = uniydb.get_settings(DEVLOC_WRITE_TIMEOUT)
        self.table_name = table_name


async def get_executor(init=None, is_remove=False):
    global _storage
    if not _storage:
        driver = await get_driver()
        ydb_config = get_ydb_config()
        _storage = Storage(driver, ydb_config['table_name'], init(ydb_config['table_name']) if init else None)
    if is_remove:
        return uniydb.YdbExecutor(_storage.remove_sessions, COUNTER_PREFIX)
    return uniydb.YdbExecutor(_storage.sessions, COUNTER_PREFIX)


class DeviceLocator:
    """ Class similar to library.messenger.client_locator
    """
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
                    logger = Logger.get('.device.locator.initialize')
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
            COUNTER_PREFIX,
            [
                DeviceLocator.REMOVE,
                DeviceLocator.STORE,
                DeviceLocator.LIST,
            ]
        )

    @staticmethod
    def make_timestamp():
        return int(time.time() * 1000000)

    @staticmethod
    def compute_shard_key(key: str):
        return CityHash64(key.encode('utf-8'))

    async def store(puid: str, device_id: str, host: str, ts, device_model, config):
        executor = await get_executor()

        # build store query parameters
        parameters = {
            '$shard_key': DeviceLocator.compute_shard_key(puid),
            '$puid': puid.encode('utf-8'),
            '$device_id': device_id.encode('utf-8'),
            '$host': host.encode('utf-8'),
            '$ts': ts,
            '$device_model': device_model.encode('utf-8'),
            '$created_at': int(datetime.now(timezone.utc).timestamp()),
            '$config': config.SerializeToString(),
        }

        async def _store(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, STORE_QUERY, _storage.table_name, _storage.w_settings)
            return await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.w_settings))

        try:
            await executor.execute(_store, DeviceLocator.STORE, DEVLOC_WRITE_TIMEOUT, retry_empty_pool=False)
            return True
        except Exception as exc:
            logger = Logger.get('.device.locator.store')
            logger.exception(exc)
            logger.error(str(exc), traceback.format_exc())

        return False

    async def list(puid: str, device_id):
        """ return all records for current puids
        """
        res = ClientEntry()
        executor = await get_executor()

        shard_key = DeviceLocator.compute_shard_key(puid)

        parameters = {
            '$shard_key': shard_key,
            '$puid': puid.encode('utf-8'),
        }

        # Logger.get().debug('{} ydb: try get list of {}'.format(id(parameters), len(puids)))

        async def _list(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, LIST_QUERY, _storage.table_name, _storage.r_settings)
            return await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.r_settings))

        retries_count = 0
        result_sets = None
        while not result_sets and retries_count < CLIENT_LOCATOR_GETS_RETRIES:
            try:
                result_sets = await executor.execute(_list, DeviceLocator.LIST, DEVLOC_READ_TIMEOUT, retry_empty_pool=False)
            except Exception as exc:
                Logger.get('.device.locator.list').exception(exc)
            finally:
                retries_count += 1

        if not result_sets:
            return res

        rows = result_sets[0].rows
        for r in rows:
            if device_id and device_id != r['device_id'].decode():
                continue
            dm = r.get('device_model')
            if dm:
                dm = dm.decode()

            config = TDeviceConfig()
            if r['config']:
                config.ParseFromString(r['config'])
            res.add(r['device_id'].decode(), r['host'].decode(), r['ts'], dm, config)

        return res

    async def remove(device_id: str, host: str, ts):
        executor = await get_executor(is_remove=True)

        parameters = {
            '$device_id': device_id.encode('utf-8'),
            '$host': host.encode('utf-8'),
            '$ts': ts,
        }

        async def _remove(session, tx):
            global _storage
            q = await uniydb.prepare_query_for_table(session, REMOVE_QUERY, _storage.table_name, _storage.w_settings)
            await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True, settings=_storage.w_settings))

        retries_count = 0
        while retries_count < CLIENT_LOCATOR_GETS_RETRIES:
            try:
                await executor.execute(_remove, DeviceLocator.REMOVE, DEVLOC_WRITE_TIMEOUT, retry_empty_pool=False)
                return True
            except Exception as exc:
                Logger.get('.device.locator.remove').exception(exc)
            finally:
                retries_count += 1
        return False
