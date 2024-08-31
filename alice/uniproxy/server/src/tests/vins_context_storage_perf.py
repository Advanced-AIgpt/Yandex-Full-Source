#! /usr/bin/env python3
import os
import random
import string
import traceback
import signal
import binascii
from collections import deque
from tornado.ioloop import IOLoop, PeriodicCallback
from tornado import gen
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalCountersUpdater
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.vins_context_storage import get_instance, connect_ydb, get_table_name
import ydb
from uuid import uuid1 as gen_guid
from tornado.concurrent import Future


def gen_random_string(n):
    return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(n))


class UserSession:
    def __init__(self):
        self.data_hash = None
        self.last_request_id = None
        self.current_operation = None


CONCURRENT_SCENARIOS = 30
HEATUP_SCENARIOS_COUNT = 50
LOAD_SAVE_DELAY_MILLIS_MIN = 5
LOAD_SAVE_DELAY_MILLIS_MAX = 300
SESSION_DATA_SIZE_MIN = 10000
SESSION_DATA_SIZE_MAX = 20000
NEW_USERS_RATIO = 0.3

scenarios = deque()
active_scenarios = 0
started_scenarios = 0
completed_scenarios = 0
new_users_count = 0
saved_uuids = []
emitted_uuids = set()
user_sessions = dict()
test_session_size = random.randint(SESSION_DATA_SIZE_MIN, SESSION_DATA_SIZE_MAX)
test_session_data = bytearray(gen_random_string(test_session_size), 'ascii')


def emit_uuid():
    while True:
        result = gen_random_string(16)
        if result not in emitted_uuids:
            emitted_uuids.add(result)
            return result


def update_active_scenarios():
    while len(scenarios) > 0 and scenarios[0].done():
        scenarios.popleft()

    while active_scenarios < CONCURRENT_SCENARIOS:
        scenario = gen.convert_yielded(run_scenario())
        IOLoop.current().add_future(scenario, lambda _: update_active_scenarios())
        scenarios.append(scenario)


def print_exception(e, indent):
    if isinstance(e, ydb.Error):
        print('{0}{1}\n{0}{2}\n{0}status: {3}'.format(' ' * indent, type(e), str(e), e.status))
        if e.issues:
            for n in e.issues:
                print_exception(n, indent + 1)
    else:
        print('{0}{1}\n{0}{2}'.format(' ' * indent, type(e), str(e)))


def random_swap(data):
    i = random.randint(0, len(data) - 1)
    j = random.randint(0, len(data) - 1)
    v = data[i]
    data[i] = data[j]
    data[j] = v


async def run_scenario():
    global started_scenarios
    global new_users_count
    global active_scenarios
    global completed_scenarios
    global user_sessions

    try:
        active_scenarios += 1
        is_new = len(saved_uuids) < 100 or random.random() < NEW_USERS_RATIO
        uuid = emit_uuid() if is_new else random.choice(saved_uuids)
        user_session = UserSession() if is_new else user_sessions[uuid]
        while True:
            op = user_session.current_operation
            if not op:
                break
            await op
        user_session.current_operation = Future()
        instance = await get_instance()
        request_id = str(gen_guid())
        accessor = instance.create_accessor("session_{0}".format(started_scenarios),
                                            {
                                                'application': {
                                                    'uuid': uuid
                                                },
                                                'header': {
                                                    'request_id': request_id,
                                                    'prev_req_id': user_session.last_request_id
                                                }
                                            },
                                            None)
        started_scenarios += 1
        data = await accessor.load()
        if is_new:
            assert data is None
            new_users_count += 1
        else:
            assert data is not None
            assert binascii.crc32(data) == user_session.data_hash
        delay_millis = random.randint(LOAD_SAVE_DELAY_MILLIS_MIN, LOAD_SAVE_DELAY_MILLIS_MAX)
        await gen.sleep(float(delay_millis) / 1000)
        random_swap(test_session_data)
        new_hash = binascii.crc32(test_session_data)
        await accessor.save(dict([('', bytes(test_session_data))]))
        user_session.data_hash = new_hash
        user_session.last_request_id = request_id
        op = user_session.current_operation
        user_session.current_operation = None
        op.set_result(None)
        if is_new:
            user_sessions[uuid] = user_session
            saved_uuids.append(uuid)
        active_scenarios -= 1
        completed_scenarios += 1
    except Exception as e:
        print('SCENARIO CRASH {0}'.format(type(e)))
        print_exception(e, 0)
        traceback.print_exc()


async def heatup():
    print('=========== heatup started ===========')
    for _ in range(HEATUP_SCENARIOS_COUNT):
        await run_scenario()
    print('=========== heatup finished =========== ')


def report():
    GlobalCountersUpdater.update()
    print('\n\n***')
    print('Users count: {0}'.format(len(saved_uuids)))
    print('Started scenarios count: {0}'.format(started_scenarios))
    print('Completed scenarios count: {0}'.format(completed_scenarios))
    print('GlobalCounter:')
    for k, v in GlobalCounter.get_metrics():
        if k.startswith('vins_context'):
            print('{0}: {1}'.format(k, v))

    percentiles = [0.7, 0.9, 0.95, 0.99, 0.999]
    print('GlobalTimings:')
    for name, buckets in GlobalTimings.get_metrics():
        if name.startswith('vins_context'):
            print('{0}: {1}'.format(name, buckets))
            total_count = 0
            for b, count in buckets:
                total_count += count
            if total_count == 0:
                continue
            percentile_index = 0
            bucket_index = 0
            prev_count = 0
            percentile_values = []
            while bucket_index < len(buckets) and percentile_index < len(percentiles):
                boundary, count = buckets[bucket_index]
                limit_boundary = buckets[bucket_index + 1][0] if bucket_index < len(buckets) - 1 else boundary
                values_fraction = float(prev_count + count) / total_count
                if values_fraction >= percentiles[percentile_index]:
                    percentile_values.append((percentiles[percentile_index], limit_boundary))
                    percentile_index += 1
                else:
                    bucket_index += 1
                    prev_count += count
            print('{0}.percentiles: {1}'.format(name, percentile_values))


async def reset():
    global active_scenarios
    global started_scenarios
    global completed_scenarios
    global new_users_count

    GlobalCounter.init()
    GlobalTimings.init()
    active_scenarios = 0
    started_scenarios = 0
    completed_scenarios = 0
    new_users_count = 0
    saved_uuids.clear()
    emitted_uuids.clear()
    user_sessions.clear()

    driver = await connect_ydb()
    with driver.table_client.session().create().transaction() as tx:
        tx.execute('DELETE FROM [{0}]'.format(get_table_name()).encode('ascii'), commit_tx=True)


async def start():
    await reset()
    await heatup()
    await reset()
    update_active_scenarios()
    await gen.sleep(10050000)


def terminate(_, __):
    print('terminating...')
    os._exit(-1)


def init():
    signal.signal(signal.SIGINT, terminate)
    signal.signal(signal.SIGTERM, terminate)

    Logger.init('vins_context_storage_perf', True)


def main():
    periodic_report = PeriodicCallback(report, 3000)
    periodic_report.start()
    IOLoop.current().spawn_callback(start)
    IOLoop.current().start()


if __name__ == '__main__':
    main()
