import datetime
import os
import time
import tornado.concurrent
import tornado.gen
import traceback

from .vault_client import VaultClient

from alice.uniproxy.library.global_counter import GlobalCounter, GlobalCountersUpdater, GlobalTimings, TimingsResolution
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger

import ydb
from kikimr.public.sdk.python import tvm
from ydb.tornado import retry_operation, as_tornado_future as as_tornado_future_
from ydb.issues import SessionPoolEmpty as YdbSessionPoolEmpty

VAULT_SECRET_UUID = config.get('vault', {}).get('secret_uuid')
DEFAULT_SESSION_POOL_SIZE = 100


class YdbCounter:
    REQS = 'requests_summ'
    RETRIES = 'retries_summ'
    FAILED = 'failed_requests_summ'
    NON_RETRY = 'non_retryable_errors_summ'
    TIMEDOUT = 'timedout_summ'
    TIME_MILLIS = 'time_millis_summ'
    REQ_SIZE = 'req_size'


def as_tornado_future(foreign_future, timeout=None):
    return as_tornado_future_(foreign_future, timeout)


def make_session_pool(driver, pool_size=DEFAULT_SESSION_POOL_SIZE, min_pool_size=None, initializer=None):
    if pool_size is None:
        pool_size = DEFAULT_SESSION_POOL_SIZE
    pz = min_pool_size or 1
    return ydb.SessionPool(driver, pool_size, initializer=initializer, min_pool_size=pz)


def get_settings(timeout):
    return ydb.BaseRequestSettings().with_timeout(timeout).with_operation_timeout(timeout)


async def retry_operation_pool_async(session_pool, callee, timeout, retry_settings=None, retry_empty_pool=True, *args, **kwargs):

    retry_settings = ydb.RetrySettings() if retry_settings is None else retry_settings
    retry_after = 0.1  # sec
    max_timeout = 5  # sec

    def _r(exc):
        raise exc
    retry_settings.unknown_error_handler = _r
    retry_settings.retry_not_found = False

    async def wrapped_callee():
        end_time = time.time() + (max_timeout if timeout is None else timeout)
        while True:
            try:
                with session_pool.checkout(blocking=False) as session:
                    return await callee(session, *args, **kwargs)
            except YdbSessionPoolEmpty:
                if not retry_empty_pool:
                    raise
                if time.time() + retry_after < end_time:
                    await tornado.gen.sleep(retry_after)
                else:
                    raise

    return await tornado.gen.with_timeout(
        datetime.timedelta(seconds=timeout),
        retry_operation(wrapped_callee, retry_settings)
    )


def increment_counter(name, d=1):
    getattr(GlobalCounter, name.upper()).increment(d)


def decrement_counter(name, d=1):
    getattr(GlobalCounter, name.upper()).decrement(d)


def set_counter(name, value):
    getattr(GlobalCounter, name.upper()).set(value)


class DurationCountersRegistry:
    counters = []

    GlobalCountersUpdater.register(lambda: DurationCountersRegistry._update())

    @classmethod
    def _update(cls):
        for c in cls.counters:
            c.update()


# TODO: check it out and may be delete, because it is useless
class DurationCounter:
    def __init__(self, name):
        self._name = name
        self._finished = False
        self._started_at = time.time()
        counters = DurationCountersRegistry.counters
        counters.append(self)
        self.index = len(counters) - 1
        self.total = 0

    def finish(self):
        if self._finished:
            return
        self.update()
        counters = DurationCountersRegistry.counters
        counters[self.index] = counters[len(counters) - 1]
        counters[self.index].index = self.index
        counters.pop()
        self._finished = True

    def update(self):
        if self._finished:
            return
        now = time.time()
        d = int(1000 * (now - self._started_at))
        self.total += d
        increment_counter(self._name, d)
        self._started_at = now

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc, traceback):
        self.finish()


async def prepare_query_for_table(session, query, table_name, settings):
    start = time.monotonic()
    query_to_prepare = query.format(table_name=table_name)
    ret = await as_tornado_future(session.async_prepare(query_to_prepare, settings))
    GlobalTimings.store('ydb_prepare_req', time.monotonic() - start)
    return ret


class YdbExecutor:
    def __init__(self, sessions_pool, counters_prefix, rt_log=None, unisystem_session_id=''):
        self._log = Logger.get('.backends.ydbexecutor')
        self._sessions_pool = sessions_pool
        self._counters_prefix = counters_prefix
        self._rt_log = rt_log
        self._unisystem_session_id = unisystem_session_id

    async def execute(self, do_query, counter, timeout, tx_mode=None, retry_empty_pool=True):
        counter = '{}_{}'.format(self._counters_prefix, counter)

        def inc_counter(name):
            increment_counter('{}_{}'.format(counter, name))

        inc_counter(YdbCounter.REQS)
        request_start_time = time.monotonic()

        async def _try_execute_transaction(session):
            try:
                with session.transaction(tx_mode=tx_mode) as tx:
                    class LocalTransaction:
                        @staticmethod
                        def async_execute(query, parameters=None, **kwargs):
                            l = 0
                            if parameters:
                                for v in parameters.values():
                                    if isinstance(v, (bytes, str)):
                                        l += len(v)
                                    elif isinstance(v, list):
                                        l += sum([len(i) for i in v if isinstance(i, (bytes, str))])
                            GlobalTimings.store(counter + '_' + YdbCounter.REQ_SIZE, l)
                            return tx.async_execute(query, parameters=parameters, **kwargs)
                    return await do_query(session, LocalTransaction)
            except Exception as e:
                message = 'ydb error, error [{0}], error type [{1}], error traceback [{2}]'
                if self._rt_log is not None:
                    self._log.info(self._unisystem_session_id,
                                    message.format(e, type(e), traceback.format_exc(5).replace("\n", "")),
                                    rt_log=self._rt_log)
                    self._rt_log('ydb_error', exc_info=1)
                else:
                    # in case for delivery
                    self._log.info(self._unisystem_session_id,
                                    message.format(e, type(e), traceback.format_exc(5).replace("\n", "")))

                locks_invalidated = isinstance(e, ydb.Aborted)
                if locks_invalidated:
                    inc_counter(YdbCounter.RETRIES)
                    raise

                reset = isinstance(e, (ydb.BadSession, ydb.ConnectionError,
                                       ydb.SessionExpired, ydb.DeadlineExceed))
                retry = isinstance(e, (ydb.Timeout, ydb.Overloaded, ydb.Unavailable, ydb.SessionPoolEmpty))
                if reset or retry:
                    inc_counter(YdbCounter.RETRIES)
                else:
                    inc_counter(YdbCounter.NON_RETRY)
                inc_counter(YdbCounter.FAILED)
                raise

        with DurationCounter(counter + "_time_millis_summ"):
            try:
                return await retry_operation_pool_async(self._sessions_pool, _try_execute_transaction, timeout, retry_empty_pool=retry_empty_pool)
            except tornado.gen.TimeoutError:
                inc_counter(YdbCounter.TIMEDOUT)
                raise
            finally:
                GlobalTimings.store(counter, time.monotonic() - request_start_time)


async def _create_credentials(parameters, ydb_token, vault_type):
    logger = Logger.get('.backends.ydb')
    tvm_client_id = config.get('client_id', None)
    tvm_service_id = parameters.get('tvm_service_id', None)
    tvm_app_secret = config.get('tvm_app_secret', None)
    if tvm_client_id and tvm_service_id and tvm_app_secret:
        logger.info('using tvm to connect to ydb, self_client_id [{0}], tvm_service_id [{1}]'.format(
                    int(tvm_client_id), tvm_service_id))
        return tvm.TvmCredentialsProvider(
            self_client_id=int(tvm_client_id),
            self_secret=tvm_app_secret,
            destination_alias='ydb',
            dsts={
                'ydb': int(tvm_service_id),
            }
        )

    if not ydb_token:
        ydb_token = os.environ.get('YDB_TOKEN')
    if not ydb_token:
        if not VAULT_SECRET_UUID:
            raise RuntimeError('YDB_TOKEN not defined')
        ('YDB_TOKEN not defined, try get token from vault')
        ydb_token = await VaultClient().get_secret_value(VAULT_SECRET_UUID, vault_type)
    if not ydb_token:
        raise RuntimeError('YDB_TOKEN not defined')
    return ydb.AuthTokenCredentials(ydb_token)


async def do_connect_ydb(parameters, ydb_token=None, vault_type='voicetech_vins_ydb_token'):
    if not parameters:
        raise RuntimeError('ydb is not configured')
    logger = Logger.get('.backends.ydb')
    connection_params = ydb.ConnectionParams(
        parameters['endpoint'],
        parameters['database'],
        credentials=await _create_credentials(parameters, ydb_token, vault_type)
    )
    logger.info('connecting to YDB {0}{1}'.format(connection_params.endpoint, connection_params.database))
    result = ydb.Driver(connection_params)
    await as_tornado_future(result.async_wait(), timeout=10)
    logger.info('YDB connected')
    return result


def register_counters_for(prefix, methods):
    _ydb_oper_counters = [
        # for operations
        YdbCounter.REQS,
        YdbCounter.RETRIES,
        YdbCounter.FAILED,
        YdbCounter.NON_RETRY,
        YdbCounter.TIMEDOUT,
        YdbCounter.TIME_MILLIS,
    ]

    _ydb_sess_counters = [
        # session
        'dead_sessions_summ',
        'sessions_waiters_ammm',
        'sessions_count_ammm',
        'sessions_hang_ammm',
        'sessions_use_time_millis',
    ]

    # make YDB counters
    for m in methods:
        for o in _ydb_oper_counters:
            name = '_'.join((prefix, m, o))
            GlobalCounter.register_counter(name)

        name = '_'.join((prefix, m))
        GlobalTimings.register_counter(name)

        name = '_'.join((prefix, m, YdbCounter.REQ_SIZE))
        GlobalTimings.register_counter(name, boundaries=TimingsResolution.REQUEST_SIZE_VALUES)

    # for sessions
    for s in _ydb_sess_counters:
        name = '_'.join((prefix, s))
        GlobalCounter.register_counter(name)
