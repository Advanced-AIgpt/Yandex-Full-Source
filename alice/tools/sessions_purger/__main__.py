import argparse
import os
import logging
import sys
import signal
import random
import time
import traceback
import Queue

import datetime
import tornado.httpserver
import tornado.ioloop
import tornado.web
import enum
import json
import ydb
from threading import Thread, Lock, Condition

logger = None

RETRIES_DELAYS = [0.01, 0.02, 0.1, 0.5, 3, 10]
QUERIES = {
    'select': '''
        DECLARE $last_shard_key AS Uint64;
        DECLARE $max_shard_key AS Uint64;
        DECLARE $last_key AS String;
        DECLARE $deadline AS Uint64;
        DECLARE $limit AS Uint64;

        $batch = (
            SELECT shard_key, key, updated_at
            FROM [{table_name}]
            WHERE shard_key = $last_shard_key AND key > $last_key AND shard_key <= $max_shard_key
            ORDER BY shard_key, key
            LIMIT $limit

            UNION ALL

            SELECT shard_key, key, updated_at
            FROM [{table_name}]
            WHERE shard_key > $last_shard_key AND shard_key <= $max_shard_key
            ORDER BY shard_key, key
            LIMIT $limit
        );

        $batch_with_limit = (
            SELECT shard_key, key, updated_at FROM $batch
            ORDER BY shard_key, key
            LIMIT $limit
        );

        $count = (
            SELECT COUNT(*) FROM $batch_with_limit
        );

        SELECT shard_key, key
        FROM $batch_with_limit
        WHERE updated_at < $deadline;

        SELECT shard_key, key
        FROM $batch_with_limit
        LIMIT 1 OFFSET Unwrap($count) - 1;
    ''',
    'delete': '''
        DECLARE $deadline AS Uint64;

        DECLARE $keys AS 'List<Struct<
            shard_key: Uint64,
            key: String
        >>';

        $expired = (
            SELECT d.shard_key AS shard_key, d.key AS key
            FROM AS_TABLE($keys) AS k
            INNER JOIN [{table_name}] AS d
            ON k.shard_key = d.shard_key AND k.key = d.key
            WHERE updated_at < $deadline
        );

        DELETE FROM [{table_name}] ON
        SELECT * FROM $expired;
    '''
}


ISO8601_DATETIME_FORMAT = '%Y-%m-%d %H:%M:%S'


class LogFormatter(logging.Formatter):
    def format(self, record):
        s = '%s %s %s: %s' % (
            record.name,
            record.levelname,
            self.formatTime(record),
            record.getMessage()
        )
        if record.exc_info:
            record.exc_text = self.formatException(record.exc_info)
        if record.exc_text:
            s = s + record.exc_text
        return s


def init_logging():
    global logger

    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setFormatter(LogFormatter())
    logger = logging.getLogger('')
    logger.setLevel(logging.INFO)
    logger.addHandler(stream_handler)


def terminate(_, __):
    print('terminating...')
    os._exit(-1)


def init_signals():
    signal.signal(signal.SIGINT, terminate)
    signal.signal(signal.SIGTERM, terminate)


@enum.unique
class EIssueCode(enum.IntEnum):
    '''
    See /arcadia/yql/core/issue/protos/issue_id.proto
    '''
    CORE_EXEC = 1060
    KIKIMR_CONSTRAINT_VIOLATION = 2012
    KIKIMR_LOCKS_INVALIDATED = 2001


class Sensor:
    def __init__(self, name, options):
        self._name = name
        options = options or {}
        self._kind = options.get('kind', 'RATE')
        self._lock = Lock()
        if self._kind == 'HIST_RATE':
            self._bounds = options.get('bounds', [])
            self._buckets = [0] * len(self._bounds)
            self._inf = 0
        else:
            self._value = 0

    def increment(self, v=1):
        with self._lock:
            self._value += v

    def set(self, v):
        with self._lock:
            if self._kind == 'HIST_RATE':
                for i in xrange(0, len(self._bounds)):
                    if v <= self._bounds[i]:
                        self._buckets[i] += 1
                        break
                else:
                    self._inf += 1
            else:
                self._value = v

    def to_dict(self):
        with self._lock:
            result = {
                'labels': {'sensor': self._name},
                'kind': self._kind
            }
            if self._kind == 'HIST_RATE':
                result['hist'] = {
                    'bounds': self._bounds,
                    'buckets': self._buckets,
                    'inf': self._inf
                }
            else:
                result['value'] = self._value
            return result


class SensorsCollection:
    def __init__(self):
        self.sensors = []

    def sensor(self, name, options=None):
        result = Sensor(name, options)
        self.sensors.append(result)
        return result

    def subgroup(self, name):
        return SensorsGroup(self, name)

    def to_dict(self):
        return [s.to_dict() for s in self.sensors]


class SensorsGroup:
    def __init__(self, collection, prefix):
        self._collection = collection
        self._prefix = prefix

    def sensor(self, name, options=None):
        return self._collection.sensor(self._prefix + '/' + name, options)

    def subgroup(self, name):
        return SensorsGroup(self._collection, self._prefix + '/' + name)


class PurgerQueryCounters:
    def __init__(self, group):
        self.count = group.sensor('count')
        self.locks_invalidated = group.sensor('locks_invalidated')
        self.non_retryable_errors = group.sensor('non_retryable_errors')
        self.failed_queries = group.sensor('failed_queries')
        self.retried_queries = group.sensor('retried_queries')
        self.duration = group.sensor('duration', {
            'kind': 'HIST_RATE',
            'bounds': [1, 5, 10, 20, 50, 100, 500, 1000, 10000, 50000]
        })
        self.execution_time = group.sensor('execution_time')


class PurgerCounters:
    def __init__(self, group):
        self.query_counters = {}
        for q in QUERIES:
            self.query_counters[q] = PurgerQueryCounters(group.subgroup(q))
        self.iterations = group.sensor('iterations')
        self.failed_iterations = group.sensor('failed_iterations')
        self.iteration_duration = group.sensor('iteration_duration', {'kind': 'DGAUGE'})
        self.purged_sessions = group.sensor('purged_sessions')
        self.purge_session_errors = group.sensor('purge_session_errors')


class SimpleThreadPool:
    def __init__(self, queue_size, threads_count):
        self._queue = Queue.Queue(queue_size)
        self._threads = []
        for i in xrange(0, threads_count):
            thread = Thread(target=self._loop, name=self.__class__.__name__ + '_' + str(i))
            thread.daemon = True
            thread.start()
            self._threads.append(thread)

    def __call__(self, f):
        self._queue.put(f)

    def join(self):
        self._queue.join()

    def get_queue_size(self):
        return self._queue.qsize()

    def _loop(self):
        while True:
            f = self._queue.get()
            try:
                f()
            except Exception as e:
                try:
                    sys.stderr.write('[{0}] simple thread pool error {1}\n{2}'.format(os.getpid(), e, traceback.format_exc()))
                    sys.stderr.flush()
                except:  # noqa
                    pass
            self._queue.task_done()


class Latch:
    def __init__(self):
        self._mutex = Lock()
        self._done = Condition(self._mutex)
        self._pending_count = 0

    def inc(self):
        self._mutex.acquire()
        try:
            self._pending_count += 1
        finally:
            self._mutex.release()

    def dec(self):
        self._done.acquire()
        try:
            pending_count = self._pending_count - 1
            if pending_count <= 0:
                if pending_count < 0:
                    logger.info('invalid pending_count [{0}]'.format(pending_count))
                    pending_count = 0
                self._done.notify_all()
            self._pending_count = pending_count
        finally:
            self._done.release()

    def join(self):
        self._done.acquire()
        try:
            while self._pending_count:
                self._done.wait()
        finally:
            self._done.release()


class VinsSessionsPurger:
    def __init__(self, driver, name, table_name, counters, max_sleep_seconds,
                 max_inactivity_seconds, start_key, last_shard_key, select_batch_size, delete_batch_size,
                 run_once, skip_first_delay, delete_threads_pool):
        self._driver = driver
        self._name = name
        self._table_name = table_name
        self._counters = counters
        self._max_sleep_seconds = max_sleep_seconds
        self._max_inactivity_seconds = max_inactivity_seconds
        self._start_key = start_key
        self._last_shard_key = last_shard_key
        self._select_batch_size = select_batch_size
        self._delete_batch_size = delete_batch_size
        self._session = None
        self._prepared_queries = {}
        self._run_once = run_once
        self._skip_first_delay = skip_first_delay
        self._session_pool = ydb.SessionPool(self._driver, size=100)
        self._delete_threads_pool = delete_threads_pool
        self._deletes_latch = Latch()
        self._thread = Thread(target=self._run)
        self._thread.start()

    def join(self):
        self._thread.join()
        logger.info('purger [{0}] joined'.format(self._name))

    def _get_session(self):
        if not self._session:
            self._session = self._driver.table_client.session().create()
        return self._session

    def _run(self):
        last_iteration_failed = False
        is_first_iteration = True
        while True:
            delay = True
            if is_first_iteration:
                is_first_iteration = False
                if self._skip_first_delay:
                    logger.info('purger [{0}], first delay skipped'.format(self._name))
                    delay = False
            if delay:
                sleep_seconds = 30 if last_iteration_failed else random.randint(10, max(30, self._max_sleep_seconds))
                logger.info('purger [{0}], chosen delay [{1}], sleeping till [{2}]'.format(
                    self._name,
                    datetime.timedelta(seconds=sleep_seconds),
                    time.strftime(ISO8601_DATETIME_FORMAT, time.localtime(time.time() + sleep_seconds))))
                time.sleep(sleep_seconds)
            deadline_seconds = int(time.time()) - self._max_inactivity_seconds
            logger.info('purger [{0}] start iteration start={2} last={3}, deadline [{1}]'.format(
                self._name,
                time.strftime(ISO8601_DATETIME_FORMAT, time.gmtime(deadline_seconds)),
                self._start_key,
                self._last_shard_key,
            ))
            last_iteration_failed = False
            request_start_time = time.time()
            purged_sessions = 0
            try:
                last_shard_key = self._start_key
                last_key = ''
                sessions_to_purge = []
                while True:
                    result_sets = self._select(last_shard_key, last_key, deadline_seconds)
                    for row in result_sets[0].rows:
                        sessions_to_purge.append(row)
                        if len(sessions_to_purge) == self._delete_batch_size:
                            self._purge_sessions(sessions_to_purge, deadline_seconds)
                            sessions_to_purge = []
                    purged_sessions += len(result_sets[0].rows)
                    if len(result_sets[1].rows) == 0:
                        logger.info('purger [{0}] iteration completed, purged sessions [{1}]'
                                    .format(self._name, purged_sessions))
                        break
                    last_row = result_sets[1].rows[0]
                    last_shard_key = last_row.shard_key
                    last_key = last_row.key

                    if last_shard_key >= self._last_shard_key:
                        logger.info('purger [{0}] iteration completed by _last_shard_key, purged sessions [{1}]'
                                    .format(self._name, purged_sessions))
                        break

                if len(sessions_to_purge) > 0:
                    self._purge_sessions(sessions_to_purge, deadline_seconds)
                self._deletes_latch.join()
            except Exception as e:
                logger.exception('purger [{0}] iteration failed, error {1}'.format(
                                 self._name, e))
                self._counters.failed_iterations.increment()
                last_iteration_failed = True
            finally:
                duration_millis = (time.time() - request_start_time) * 1000
                self._counters.iterations.increment()
                self._counters.iteration_duration.set(duration_millis)

            if self._run_once:
                break

    def _select(self, last_shard_key, last_key, deadline_seconds):
        # setting max_shard_key to last_shard_key + delta make range query more lightweight
        max_shard_key = last_shard_key + (1 << 64) / 5000
        if max_shard_key >= (1 << 64):
            max_shard_key = (1 << 64) - 1
        parameters = {'$last_shard_key': last_shard_key,
                      '$max_shard_key': max_shard_key,
                      '$last_key': last_key.encode('ascii'),
                      '$deadline': deadline_seconds,
                      '$limit': self._select_batch_size}
        return self._do_with_retry('select', parameters, ydb.StaleReadOnly())

    def _purge_sessions(self, sessions_to_purge, deadline_seconds):
        def _run():
            try:
                parameters = {'$deadline': deadline_seconds,
                              '$keys': sessions_to_purge}
                self._do_with_retry('delete', parameters, ydb.SerializableReadWrite())
                self._counters.purged_sessions.increment(len(sessions_to_purge))
            except Exception as e:
                logger.exception('purge session error, purger [{0}], error {1}'.format(
                                 self._name, e))
                self._counters.purge_session_errors.increment()
            self._deletes_latch.dec()

        self._deletes_latch.inc()
        self._delete_threads_pool(_run)

    def _do_with_retry(self, name, parameters, tx_mode):
        def callee(session):
            query = QUERIES[name].format(table_name=self._table_name).encode('ascii')
            prepared = session.prepare(query)
            return session.transaction(tx_mode).execute(prepared, parameters, commit_tx=True)

        counters = self._counters.query_counters[name]
        request_start_time = time.time()
        counters.count.increment()
        try:
            return self._session_pool.retry_operation_sync(callee)
        finally:
            duration_millis = (time.time() - request_start_time) * 1000
            counters.duration.set(duration_millis)
            counters.execution_time.increment(duration_millis)


class PingHandler(tornado.web.RequestHandler):
    def get(self):
        self.write('pong')


class CountersHandler(tornado.web.RequestHandler):
    def __init__(self, *args, **kwargs):
        self._sensors = None
        self._on_update = None
        super(CountersHandler, self).__init__(*args, **kwargs)

    def initialize(self, sensors, on_update):
        self._sensors = sensors
        self._on_update = on_update

    def get(self):
        self.set_header('Content-Type', 'application/json;charset=utf-8')
        if self._on_update:
            self._on_update()
        data = {'sensors': self._sensors.to_dict()}
        self.write(json.dumps(data, ensure_ascii=False))


PROD_CONFIG = {
    "iva1-0831": {
        "shard_keys": {
            "first": 0,
            "last": 3689348814741910323
        }
    },
    "sas1-0142": {
        "shard_keys": {
            "first": 3689348814741910323,
            "last": 7378697629483820646
        }
    },
    "myt1-0663": {
        "shard_keys": {
            "first": 7378697629483820646,
            "last": 11068046444225730969
        }
    },
    "man2-2271": {
        "shard_keys": {
            "first": 11068046444225730969,
            "last": 14757395258967641292
        }
    },
    "vla2-5876": {
        "shard_keys": {
            "first": 14757395258967641292,
            "last": 18446744073709551615
        }
    }
}


def get_config():
    ihost = os.environ.get('BSCONFIG_IHOST')
    if ihost is None:
        return {}
    return PROD_CONFIG.get(ihost, {})


def get_default_start_key():
    return int(get_config().get('shard_keys', {}).get('first', 0))


def get_default_last_key():
    return int(get_config().get('shard_keys', {}).get('last', int((1 << 64) - 1)))


def main():
    '''Universal tool for deleting records in YDB that are older than max-sleep-minutes from now().
    Target table must have key_columns as '(shard_key, key)' and column 'updated_at' with upload timestamp.
    '''
    init_logging()
    init_signals()

    default_start_key = get_default_start_key()
    default_last_key = get_default_last_key()

    try:
        parser = argparse.ArgumentParser(
            description=main.__doc__,
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        parser.add_argument('--target',
                            metavar='name=connection_string',
                            help='database to monitor, format: name=endpoint/db[:table], can be specified multiple times',
                            action='append',
                            required=True)
        parser.add_argument('--max-sleep-minutes',
                            help='sleep for random period in this interval',
                            default=5 * 60,
                            type=int)
        parser.add_argument('--max-inactivity-hours',
                            help='kill session if it has not been updated for the long time',
                            default=24 * 2,
                            type=int)
        parser.add_argument('--start-key',
                            help='start shard key to purge',
                            default=default_start_key,
                            type=int)
        parser.add_argument('--last-key',
                            help='last shard key to purge',
                            default=default_last_key,
                            type=int)
        parser.add_argument('--skip-first-delay',
                            help='run first iteration immediately',
                            default=False,
                            action='store_true')
        parser.add_argument('--run-once',
                            help='exit immediately after first run',
                            default=False,
                            action='store_true')
        parser.add_argument('--select-batch-size', default=500, type=int)
        parser.add_argument('--delete-batch-size', default=100, type=int)
        parser.add_argument('--delete-queue-size', default=100, type=int)
        parser.add_argument('--delete-threads-pool-size', default=5, type=int)

        parser.add_argument('--port', help='monitoring port', default=80, type=int)
        args = parser.parse_args()

        ydb_token = os.environ.get('YDB_TOKEN')
        if not ydb_token:
            raise RuntimeError('YDB_TOKEN not defined, set it via environment variable')

        sensors = SensorsCollection()
        delete_threads_pool = SimpleThreadPool(args.delete_queue_size, args.delete_threads_pool_size)
        delete_threads_pool_queue_size_sensor = sensors.sensor('delete_threads_pool_queue_size', {'kind': 'DGAUGE'})

        def update_thread_pools_sensors():
            delete_threads_pool_queue_size_sensor.set(delete_threads_pool.get_queue_size())

        http_application = tornado.web.Application([
            ('/ping', PingHandler),
            ('/counters', CountersHandler, dict(sensors=sensors, on_update=update_thread_pools_sensors))
        ])
        http_server = tornado.httpserver.HTTPServer(http_application)
        http_server.bind(args.port)
        http_server.start()

        purgers = []
        for t in args.target:
            name, _, v = t.partition('=')
            endpoint, _, db = v.partition('/')
            db = '/' + db
            table = 'data'
            if ':' in db:
                db, _, table = db.partition(':')
            logger.info('purger [{0}], db [{1}], endpoint [{2}], table [{3}]'.format(name, db, endpoint, table))
            connection_params = ydb.ConnectionParams(endpoint, db, auth_token=ydb_token)
            logger.info('connecting to {0}{1}'.format(connection_params.endpoint, connection_params.database))
            driver = ydb.Driver(connection_params)
            driver.wait()
            logger.info('connected')

            purgers.append(VinsSessionsPurger(driver,
                                              name,
                                              '/'.join([db, table]),
                                              PurgerCounters(sensors.subgroup(name)),
                                              args.max_sleep_minutes * 60,
                                              args.max_inactivity_hours * 60 * 60,
                                              args.start_key,
                                              args.last_key,
                                              args.select_batch_size,
                                              args.delete_batch_size,
                                              args.run_once,
                                              args.skip_first_delay,
                                              delete_threads_pool))
        logger.info('purgers started')
        if args.run_once:
            for purger in purgers:
                purger.join()
            delete_threads_pool.join()
        else:
            tornado.ioloop.IOLoop.current().start()
    except Exception as e:
        logger.exception('unhandled error: %s', e)


if __name__ == '__main__':
    main()
