import datetime
import hashlib
import tornado.ioloop
import tornado.concurrent
import tornado.gen
from tornado.concurrent import Future
from rtlog import null_logger

import concurrent.futures

from ydb import Error as YdbError

import alice.uniproxy.library.backends_common.ydb as uniydb
from alice.uniproxy.library.backends_common.protohelpers import proto_to_json
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config

from voicetech.library.proto_api.yabio_pb2 import YabioContext

UPSERT_QUERY = '''
        DECLARE $shard_key as Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;
        DECLARE $context as String;
        DECLARE $updated_at as Uint64;
        DECLARE $version as Uint32;

        SELECT updated_at
            FROM [{table_name}]
            WHERE shard_key = $shard_key AND
                group_id = $group_id AND
                dev_model = $dev_model AND
                dev_manuf = $dev_manuf;

        UPSERT INTO [{table_name}]
        (shard_key, group_id, dev_model, dev_manuf, context, updated_at, version)
        VALUES
        ($shard_key, $group_id, $dev_model, $dev_manuf, $context, $updated_at, $version);

        SELECT 1;
    '''

SELECT_QUERY = '''
        DECLARE $shard_key AS Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;

        SELECT context, updated_at, version
            FROM [{table_name}]
            WHERE shard_key = $shard_key AND
                group_id = $group_id AND
                dev_model = $dev_model AND
                dev_manuf = $dev_manuf;
    '''

LIST_QUERY = '''
        DECLARE $off as Uint32;
        DECLARE $upd as Uint64;

        SELECT group_id, dev_model, dev_manuf
            FROM [{table_name}]
            WHERE updated_at >= $upd
            LIMIT 1000
            OFFSET $off;
    '''

DELETE_QUERY = '''
        DECLARE $shard_key AS Uint64;
        DECLARE $group_id as String;
        DECLARE $dev_model as String;
        DECLARE $dev_manuf as String;

        DELETE FROM [{table_name}]
            WHERE shard_key = $shard_key AND
                group_id = $group_id AND
                dev_model = $dev_model AND
                dev_manuf = $dev_manuf;
    '''

YABIO_READ_TIMEOUT = config.get('yabio', {}).get('context_storage', {}).get('read_timeout', 0.5)
YABIO_WRITE_TIMEOUT = config.get('yabio', {}).get('context_storage', {}).get('write_timeout', 1.5)

_ARGS = None


def get_key(*args, **kwargs):
    parts = args
    if len(kwargs) > 0:
        parts = [kwargs['group_id'], kwargs['dev_model'], kwargs['dev_manuf']]
    try:
        return b'_'.join([p.encode('utf-8') if isinstance(p, str) else p for p in parts])
    except Exception:
        Logger.get('.yabio_storage').error("can't get key from {}", parts)
        return b''


class YabioStorageAccessor:
    SELECT = 'select'
    UPSERT = 'upsert'
    DELETE = 'delete'

    @staticmethod
    def init_counters():
        uniydb.register_counters_for(
            'yabio_storage',
            [
                YabioStorageAccessor.SELECT,
                YabioStorageAccessor.UPSERT,
                YabioStorageAccessor.DELETE,
            ]
        )

    def __init__(self, sessions_pool, table_name, rt_log=null_logger(), unisystem_session_id='',
                 group_id=None, dev_model=None, dev_manuf=None):
        self._log = Logger.get('.yabio_storage')
        self._group_id = (group_id.encode('utf-8') if isinstance(group_id, str) else group_id).lower()
        self._dev_model = (dev_model.encode('utf-8') if isinstance(dev_model, str) else dev_model).lower()
        self._dev_manuf = (dev_manuf.encode('utf-8') if isinstance(dev_manuf, str) else dev_manuf).lower()

        key = get_key(self._group_id, self._dev_model, self._dev_manuf)
        self._shard_key = self._compute_shard_key(key)
        self._session_id = unisystem_session_id
        self._record_ts = None
        self.rt_log = rt_log
        self._table_name = table_name
        self.r_settings = uniydb.get_settings(YABIO_READ_TIMEOUT)
        self.w_settings = uniydb.get_settings(YABIO_WRITE_TIMEOUT)
        self._executor = uniydb.YdbExecutor(sessions_pool, 'yabio_storage', self.rt_log, unisystem_session_id)

    async def save(self, context: bytes):
        new_ts = int(datetime.datetime.utcnow().timestamp())
        # build upsert query parameters
        parameters = {
            '$shard_key': self._shard_key,
            '$group_id': self._group_id,
            '$dev_model': self._dev_model,
            '$dev_manuf': self._dev_manuf,
            '$context': context,
            '$updated_at': new_ts,
            '$version': 1,
        }

        async def _upsert(session, tx):
            q = await uniydb.prepare_query_for_table(session, UPSERT_QUERY, self._table_name, self.w_settings)
            result_sets = await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True,
                                                                          settings=self.w_settings))
            if len(result_sets[1].rows) == 0:
                raise Exception('nothing saved (reason unknown)')
            if (
                self._record_ts is not None
                and result_sets[0].rows
                and result_sets[0].rows[0].get('updated_at') is not None
                and result_sets[0].rows[0]['updated_at'] != self._record_ts
            ):
                self.WARN('yabio_storage_conflicts overwrite foreign record with ts={} with new record ts={}'.format(
                    result_sets[0].rows[0]['updated_at'],
                    new_ts,
                ))
                uniydb.increment_counter('yabio_storage_conflicts_summ')

        await self._executor.execute(_upsert, self.UPSERT, YABIO_WRITE_TIMEOUT)
        self._record_ts = new_ts

    async def load(self):
        """ return all records for current group_id
        """
        parameters = {
            '$shard_key': self._shard_key,
            '$group_id': self._group_id,
            '$dev_model': self._dev_model,
            '$dev_manuf': self._dev_manuf,
        }

        async def _query(session, tx):
            q = await uniydb.prepare_query_for_table(session, SELECT_QUERY, self._table_name, self.r_settings)
            return await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True,
                                                                   settings=self.r_settings))

        result_sets = await self._executor.execute(_query, self.SELECT, YABIO_READ_TIMEOUT)
        rows = result_sets[0].rows
        if rows:
            self._record_ts = rows[0]['updated_at']
        return rows

    async def delete(self):
        parameters = {
            '$shard_key': self._shard_key,
            '$group_id': self._group_id,
            '$dev_model': self._dev_model,
            '$dev_manuf': self._dev_manuf,
        }

        async def _delete(session, tx):
            q = await uniydb.prepare_query_for_table(session, DELETE_QUERY, self._table_name, self.w_settings)
            await uniydb.as_tornado_future(tx.async_execute(q, parameters, commit_tx=True,
                                                            settings=self.w_settings))

        await self._executor.execute(_delete, self.DELETE, YABIO_WRITE_TIMEOUT)

    @staticmethod
    def _compute_shard_key(key):
        return int(hashlib.md5(key).hexdigest(), 16) & ((1 << 64) - 1)

    def DLOG(self, *args):
        self._log.debug(self._session_id, *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self._session_id, *args, rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self._session_id, *args, rt_log=self.rt_log)

    def ERR(self, *args):
        self._log.error(self._session_id, *args, rt_log=self.rt_log)

    def EXC(self, text):
        self._log.exception(self._session_id + ' ' + text, rt_log=self.rt_log)


class YabioStorage:
    def __init__(self, driver, table_name, pool_size):
        Logger.get('.yabio_storage').info('connect to table {}'.format(table_name))
        self._sessions = uniydb.make_session_pool(driver, pool_size=pool_size)
        self._table_name = table_name

    def create_accessor(self, group_id, dev_model, dev_manuf, rt_log=null_logger()):
        return YabioStorageAccessor(
            self._sessions, self._table_name, rt_log=rt_log,
            group_id=group_id, dev_model=dev_model, dev_manuf=dev_manuf
        )


_instance = None
_context_storage_config = config["yabio"].get("context_storage")


async def connect_ydb(ydb_token=None):
    return await uniydb.do_connect_ydb(_context_storage_config, ydb_token)


async def get_instance(table_name):
    global _instance
    if not _instance:
        driver = await connect_ydb()
        _instance = YabioStorage(driver, table_name, _context_storage_config.get('pool_size'))
    return _instance


class ContextStorage:
    def __init__(self, session_id, rt_log=None, group_id=None, dev_model=None, dev_manuf=None, table_name=None):
        self._log = Logger.get('.yabio_storage')
        self.group_id = group_id
        self.session_id = session_id
        self.rt_log = rt_log
        self.dev_model = dev_model
        self.dev_manuf = dev_manuf
        self.key = get_key(group_id, dev_model, dev_manuf)

        self.ydb_accessor = None
        self.yabio_context = None
        self.enrolling_limit = 40
        self.mds_url_cache = {}
        self.text_cache = {}
        self._load_future = None
        self._table_name = table_name or _context_storage_config['table_name']
        self._save_future = None
        self._already_wait_until_save = False

    @tornado.gen.coroutine
    def ensure_context_exist(self):
        if self.yabio_context is None:
            # for concurrent loading
            if self._load_future is None:
                self._load_future = self.load()
            try:
                yield self._load_future
            except:  # noqa
                self._load_future = None
                raise

    @tornado.gen.coroutine
    def load(self):
        try:
            yabio_storage = yield get_instance(self._table_name)
        except (TimeoutError, concurrent.futures.TimeoutError, tornado.gen.TimeoutError):
            raise Exception('timeout on connect to YDB')

        if not self.ydb_accessor:
            self.ydb_accessor = yabio_storage.create_accessor(
                group_id=self.group_id, dev_model=self.dev_model, dev_manuf=self.dev_manuf,
                rt_log=self.rt_log
            )
        recs = yield self.ydb_accessor.load()
        self.INFO('loaded yabio key={}'.format(self.key))
        if len(recs) < 1:
            self.yabio_context = YabioContext(group_id=self.group_id)
        else:
            try:
                data = recs[0]['context']
                ctx = YabioContext()
                ctx.ParseFromString(data)
                self.yabio_context = ctx
            except Exception as exc:
                self.WARN('fail parse yabio context key={} from ydb (replace with new/empty): {}'.format(
                    self.key, exc))
                self.yabio_context = YabioContext(group_id=self.group_id)

    @tornado.gen.coroutine
    def save(self):
        if self._save_future is None:
            self._save_future = Future()
        else:
            if self._already_wait_until_save:
                self.DLOG('another coroutine already waits to save data')
                return

            self._already_wait_until_save = True
            fut = self._save_future
            yield fut
            self._already_wait_until_save = False

        try:
            yield self.ensure_context_exist()
            re_try = config["yabio"].get('re_try_save', 2)
            while True:
                try:
                    yield self.ydb_accessor.save(self.yabio_context.SerializeToString())
                    self.INFO('saved yabio key={}'.format(self.key))
                    return  # success
                except YdbError as exc:
                    self.WARN('YDB fail on save yabio context key={} (re-tries left={}): {}'.format(
                        self.key, re_try, str(exc)))
                    if re_try <= 0:
                        raise  # final fail
                    re_try -= 1
        finally:
            self._save_future.set_result(True)
            self._save_future = None

    def add_text(self, request_id, source, text):
        self.text_cache[(request_id, source)] = text

    @tornado.gen.coroutine
    def update_mds_url(self, request_id, source, mds_url):
        yield self.ensure_context_exist()

        found = False
        for enroll in self.yabio_context.enrolling:
            if enroll.request_id == request_id and enroll.source == source:
                found = True
                enroll.mds_url = mds_url

        if self.yabio_context.users:
            for user in self.yabio_context.users:
                for vp in user.voiceprints:
                    if vp.request_id == request_id and vp.source == source:
                        found = True
                        vp.mds_url = mds_url

        self.mds_url_cache[(request_id, source)] = mds_url
        if not found:
            return

        yield self.save()

    @tornado.gen.coroutine
    def get_users_context(self):
        """ get context without enrolling (for sending to yabio server)
        """
        yield self.ensure_context_exist()
        if len(self.yabio_context.enrolling):
            ctx = YabioContext()
            ctx.CopyFrom(self.yabio_context)
            del ctx.enrolling[:]
            return ctx

        return self.yabio_context

    @tornado.gen.coroutine
    def save_new_enrolling(self, enrollings, compatibility_tags=None):
        """ add yabio server score response (fresh enrolling(s)) to context
            (limit context enrollings size, remove old records with same request_id as new)
        """
        # can have few records with same request_id (spotter+request), so mixin soure for generate uniq rec_id
        # & rewrite(remove) only new records
        fresh_rec_ids = set((e.request_id, e.source) for e in enrollings)
        self.DLOG('save {} new enrollings={}'.format(len(enrollings), fresh_rec_ids))
        yield self.ensure_context_exist()

        # delete unsupported compatibility tags from db
        if compatibility_tags:
            iu = 0
            while iu < len(self.yabio_context.users):
                i = 0
                u = self.yabio_context.users[iu]
                while i < len(u.voiceprints):
                    e = u.voiceprints[i]
                    if e.compatibility_tag not in compatibility_tags:
                        del u.voiceprints[i]
                    else:
                        i += 1

                if len(u.voiceprints) == 0:
                    del self.yabio_context.users[iu]
                else:
                    iu += 1

        i = 0
        while i < len(self.yabio_context.enrolling):
            e = self.yabio_context.enrolling[i]
            if (compatibility_tags and e.compatibility_tag not in compatibility_tags) \
               or (e.request_id, e.source) in fresh_rec_ids:
                # has fresh record, so remove old
                del self.yabio_context.enrolling[i]
            else:
                i += 1

        for enroll in enrollings:
            mds_url = self.mds_url_cache.get((enroll.request_id, enroll.source))
            if mds_url:
                enroll.mds_url = mds_url

            text = self.text_cache.get((enroll.request_id, enroll.source))
            if text:
                enroll.text = text

        self.yabio_context.enrolling.extend(enrollings)

        overlimit = len(self.yabio_context.enrolling) - self.enrolling_limit
        if overlimit > 0:
            del self.yabio_context.enrolling[:overlimit]
        self.DLOG('now have {} enrollings'.format(len(self.yabio_context.enrolling)))

        yield self.save()

    @tornado.gen.coroutine
    def add_user(self, user_id, requests_ids):
        self.INFO('add user={} with requests_ids={}'.format(user_id, requests_ids))
        yield self.ensure_context_exist()

        user = None
        for u in self.yabio_context.users:
            if u.user_id == user_id:
                user = u
                break

        required_requests_ids = set(requests_ids)
        added_request_ids = set()
        new_voiceprints = []
        for e in self.yabio_context.enrolling:
            if e.request_id in required_requests_ids:
                e.reg_num = requests_ids.index(e.request_id)
                new_voiceprints.append(e)
                added_request_ids.add(e.request_id)

        if not new_voiceprints:
            raise Exception('not found all requested voiceprints for creating new user')

        lost_vp = len(required_requests_ids) - len(added_request_ids)
        if lost_vp:
            self.WARN('yabio_contex not found all request_id for create/update new user lost={}'.format(lost_vp))

        if not user:
            self.INFO('yabio_context not found exist user, create new for user_id={}'.format(user_id))
            user = self.yabio_context.users.add()
            user.user_id = user_id
        else:
            del user.voiceprints[:]

        user.voiceprints.extend(new_voiceprints)

        # remove used for user enrolling voiceprints
        i = 0
        while i < len(self.yabio_context.enrolling):
            if self.yabio_context.enrolling[i].request_id in requests_ids:
                del self.yabio_context.enrolling[i]
            else:
                i += 1

        yield self.save()

    @tornado.gen.coroutine
    def get_users(self):
        yield self.ensure_context_exist()
        # old format (now we not have mode_id):
        # "users": [{"user_id": x.user_id, "model_id": x.model_id} for x in res.voiceprint]
        users = []
        for user in self.yabio_context.users:
            users.append({'user_id': user.user_id})
        return users

    @tornado.gen.coroutine
    def remove_user(self, user_id):
        yield self.ensure_context_exist()
        user_removed = False
        i = 0
        while i < len(self.yabio_context.users):
            if self.yabio_context.users[i].user_id == user_id:
                del self.yabio_context.users[i]
                user_removed = True
            else:
                i += 1
        if user_removed:
            yield self.save()

    @tornado.gen.coroutine
    def remove_users(self):
        yield self.ensure_context_exist()
        if self.yabio_context.users:
            while len(self.yabio_context.users):
                del self.yabio_context.users[0]
            yield self.save()

    @tornado.gen.coroutine
    def update_users_voiceprints(self, user_voiceprints):
        yield self.ensure_context_exist()

        for user in self.yabio_context.users:
            voiceprints = user_voiceprints.get(user.user_id, [])

            for v in voiceprints:
                v1 = (v['request_id'], v['compatibility_tag'], v['source'])
                v2 = (v['request_id'], v['source'])
                if not any(v1 == (uv.request_id, uv.compatibility_tag, uv.source)
                           for uv in user.voiceprints) \
                   and any(v2 == (uv.request_id, uv.source)
                           for uv in user.voiceprints):
                    new_v = user.voiceprints.add()
                    new_v.request_id = v['request_id']
                    new_v.compatibility_tag = v['compatibility_tag']
                    new_v.format = v['format']
                    new_v.source = v['source']
                    new_v.mds_url = v['mds_url']
                    new_v.voiceprint[:] = v['voiceprint']

        yield self.save()

    def log_hr(self):
        self._log.debug(self.session_id, proto_to_json(self.yabio_context), rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self.session_id, *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self.session_id, *args, rt_log=self.rt_log)

    def DLOG(self, *args):
        self._log.debug(self.session_id, *args, rt_log=self.rt_log)


def get_all_keys(sessions_pool, table_name, settings, upd_at=0):
    keys = []
    offset = 0
    ended = False

    def _execute_transaction(session, offset):
        q = uniydb.prepare_query_for_table(session, LIST_QUERY, table_name, settings)
        parameters = {'$off': offset, '$upd': upd_at}
        with session.transaction() as tx:
            result_sets = tx.execute(q, parameters, commit_tx=True)
            rows = result_sets[0].rows
            ended = (len(rows) == 0)
            keys.extend(list(map(get_key, rows)))
            offset += len(rows)
            Logger.get().info('read: {}'.format(offset))
            return ended, offset

    while not ended:
        ended, offset = sessions_pool.retry_operation_sync(_execute_transaction, None, offset)

    Logger.get().info('Total number of keys: {}'.format(offset))
    return keys
