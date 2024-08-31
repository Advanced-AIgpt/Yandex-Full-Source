#!/usr/bin/env python
"""
import tornado.gen
import tornado.ioloop

from log import DLOG, init_logging

import uuid
import json

from messenger.locator import ClientLocator

from alice.uniproxy.library.global_counter import GlobalCounter

from alice.uniproxy.library.backends_memcached import memcached_init as init_mc
from alice.uniproxy.library.backends_memcached import memcached_client
from alice.uniproxy.library.backends_memcached import MEMCACHED_MSSNGR_SERVICE


# ====================================================================================================================
def _run_locator_test(test_fn):
    GlobalCounter.init()
    init_logging('test.locator', False)
    init_mc()

    ioloop = tornado.ioloop.IOLoop.current()
    mc = memcached_client(MEMCACHED_MSSNGR_SERVICE)

    result = True, ''

    @tornado.gen.coroutine
    def test_fn_wrapper(mc):
        nonlocal result
        try:
            yield test_fn(mc)
        except tornado.gen.Return as ex:
            pass
        except Exception as ex:
            result = False, str(ex)
        finally:
            ioloop.stop()

    ioloop.spawn_callback(test_fn_wrapper, mc)
    ioloop.start()

    assert result == (True, '')


# ====================================================================================================================
def _test_guid_mapping():

    @tornado.gen.coroutine
    def main(memcached):
        TEST_UUID_COUNT = 10
        locator = ClientLocator.instance()

        yield memcached.xdelete('test-guid')
        value = yield memcached.xget('test-guid')
        assert value is None

        _uuids = [
            str(uuid.uuid4()) for x in range(TEST_UUID_COUNT)
        ]

        results = []
        for i in range(TEST_UUID_COUNT):
            result = yield locator.update_location('test-guid', _uuids[i])
            results.append(result)

        assert all(map(lambda x: all(x), results))

        value = yield memcached.get('test-guid', server=0)
        assert value is not None
        assert len(value.split(';')) == 5

        value = yield memcached.get('test-guid', server=1)
        assert value is not None
        assert len(value.split(';')) == 5

        locations = yield locator.resolve_by_guid(['test-guid'])
        DLOG(locations)

        _resolved_uuids = []
        for _, _uuid, _ in locations:
            _resolved_uuids.append(_uuid)

        expected_result = _uuids[-5:]
        expected_result.sort()
        print(expected_result)

        _resolved_uuids.sort()
        print(_resolved_uuids)

        r = _uuids == _resolved_uuids
        raise tornado.gen.Return((r, str(r)))

    _run_locator_test(main)


# ====================================================================================================================
def _test_uuid_eviction():

    @tornado.gen.coroutine
    def main(memcached):
        UUID = uuid.uuid4()
        GUID = 'test-eviction-%s' % (UUID)
        UUIDS = [
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-2' % (UUID),
            'test-eviction-%s-3' % (UUID),
            'test-eviction-%s-4' % (UUID),
            'test-eviction-%s-5' % (UUID),
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-9' % (UUID),
            'test-eviction-%s-8' % (UUID),
            'test-eviction-%s-7' % (UUID),
            'test-eviction-%s-6' % (UUID),
        ]

        locator = ClientLocator.instance()

        yield memcached.xset(GUID, '', exptime=5)

        for uuid_ in UUIDS:
            yield memcached.xset(uuid_, uuid_, exptime=5)
            yield locator.update_location(GUID, uuid_)

        resolved_locations = yield locator.resolve_by_guid([GUID])
        expected_locations = UUIDS[-5:]
        test_log = open('test.log', 'w')
        test_log.write('expected_locations => %s\n' % (expected_locations))
        test_log.write('resolved_locations => %s\n' % (resolved_locations))
        test_log.flush()

        r = True
        for guid_, uuid_, location in resolved_locations:
            test_log.write('GUID %s UUID %s => %s\n' % (guid_, uuid_, location))
            test_log.flush()

            if uuid_ not in expected_locations:
                raise Exception('unexpected location found: %s' % uuid_)
            else:
                print('%s -> %s : %s' % (uuid_, location, location in expected_locations))

        test_log.close()

        raise tornado.gen.Return((r, str(resolved_locations)))

    _run_locator_test(main)


# ====================================================================================================================
def _test_uuid_eviction2():

    @tornado.gen.coroutine
    def main(memcached):
        UUID = uuid.uuid4()
        GUID = 'test-eviction-%s' % (UUID)
        UUIDS = [
            'test-eviction-%s-9' % (UUID),
            'test-eviction-%s-8' % (UUID),
            'test-eviction-%s-7' % (UUID),
            'test-eviction-%s-6' % (UUID),
            'test-eviction-%s-2' % (UUID),
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-1' % (UUID),
            'test-eviction-%s-1' % (UUID),
        ]
        EXPECTED = [
            'test-eviction-%s-8' % (UUID),
            'test-eviction-%s-7' % (UUID),
            'test-eviction-%s-6' % (UUID),
            'test-eviction-%s-2' % (UUID),
            'test-eviction-%s-1' % (UUID),
        ]

        locator = ClientLocator.instance()

        yield memcached.xset(GUID, '', exptime=5)

        for uuid_ in UUIDS:
            yield memcached.xset(uuid_, uuid_, exptime=5)
            yield locator.update_location(GUID, uuid_)

        resolved_locations = yield locator.resolve_by_guid([GUID])
        expected_locations = UUIDS[-5:]
        test_log = open('test.log', 'w')
        test_log.write('expected_locations => %s\n' % (expected_locations))
        test_log.write('resolved_locations => %s\n' % (resolved_locations))
        test_log.flush()

        r = True

        resolved = [uuid_ for guid, uuid_, location_ in resolved_locations]
        for uuid_ in EXPECTED:
            if uuid_ not in resolved:
                raise Exception('missing uuid: %s' % uuid_)

        test_log.close()

        raise tornado.gen.Return((r, str(resolved_locations)))

    _run_locator_test(main)


if __name__ == '__main__':
    _test_uuid_eviction()
"""
