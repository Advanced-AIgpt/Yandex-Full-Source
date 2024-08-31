import uuid as uuid_
import json
import logging
import struct

import tornado.gen

import alice.uniproxy.library.global_state
import alice.uniproxy.library.global_counter
import alice.uniproxy.library.testing

from alice.uniproxy.library.delivery.handler import DeliveryHandler
from alice.uniproxy.library.delivery.server import DeliveryServer

from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage
from mssngr.router.lib.protos.message_pb2 import TOutMessage

from subway.server.subway_server import SubwayServer
from subway.client.pull import SubwayPullClient
from subway.client.push import g_locations_remover

from clickhouse_cityhash.cityhash import CityHash64

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)12s: %(name)-16s %(message)s')


alice.uniproxy.library.global_counter.GlobalCounter.init()
alice.uniproxy.library.global_state.GlobalState.init(1)


# ====================================================================================================================
def uuidgen():
    return alice.uniproxy.library.testing.uuidgen()


# ====================================================================================================================
class MockUnisystem:
    def __init__(self, session_id=None, uuid=None, guid=None):
        self.session_id = session_id if session_id else uuidgen()
        self._uuid = uuid if uuid else uuidgen()
        self.guid = guid if guid else uuidgen()

    def uuid(self):
        return self._uuid


# ====================================================================================================================
class MockClientEntry(object):
    def __init__(self, *args, **kwargs):
        super(MockClientEntry, self).__init__()
        self.uuids = []
        self.cids = []

    # ----------------------------------------------------------------------------------------------------------------
    def add_uuid(self, uuid):
        self.uuids.append(uuid)

    # ----------------------------------------------------------------------------------------------------------------
    def add_client_id(self, client_id):
        if isinstance(client_id, str):
            self.cids.append(uuid_.UUID(client_id).bytes)
        elif isinstance(client_id, bytes):
            self.cids.append(client_id)

    # ----------------------------------------------------------------------------------------------------------------
    def enumerate_locations(self, binary_client_id=False):
        for uuid in self.uuids:
            yield uuid, None, 'localhost'

        for cid in self.cids:
            if binary_client_id:
                yield None, cid, 'localhost', 0
            else:
                yield None, str(uuid_.UUID(bytes=cid)), 'localhost', 0


# ====================================================================================================================
g_locator_instance = None


class MockClientLocator(object):
    def __init__(self, *args, **kwargs):
        super(MockClientLocator, self).__init__()
        self._log = logging.getLogger('delivery.test.locator')
        self.map = {}

    def add_uuid(self, guid, uuid):
        self._log.debug('GUID(%s) => UUID(%s)', guid, uuid)
        if guid not in self.map:
            self.map[guid] = MockClientEntry()
        self.map[guid].add_uuid(uuid)

    def add_client_id(self, guid, client_id):
        self._log.debug('GUID(%s) => SESSION(%s)', guid, client_id)
        if guid not in self.map:
            self.map[guid] = MockClientEntry()
        self.map[guid].add_client_id(client_id)

    @tornado.gen.coroutine
    def resolve_locations(self, guids):
        results = {}
        for guid, entry in self.map.items():
            if guid in guids:
                results[guid] = entry

        for guid, entry in results.items():
            for uuid, cid, location, index in entry.enumerate_locations():
                self._log.info('RESOLVED GUID(%s) UUID(%s) CID(%s) LOC(%s)', guid, uuid, cid, location)
        return results

    def reset(self):
        self.map = {}

    @staticmethod
    def instance():
        global g_locator_instance
        if g_locator_instance is None:
            g_locator_instance = MockClientLocator()
        return g_locator_instance


# ====================================================================================================================
class MockDeliveryHandler(DeliveryHandler):
    def __init__(self, request, *args, **kwargs):
        self._port = kwargs.get('subway_port', 7789)
        if 'subway_port' in kwargs:
            del kwargs['subway_port']

        super(MockDeliveryHandler, self).__init__(request, *args, **kwargs)

    def prepare(self):
        self._service_ticket = self.request.headers.get('X-Ya-Service-Ticket')
        self._subway_port = self.request.headers.get('X-Subway-Port', self._port)
        self._subway_format = self.request.headers.get('X-Subway-Format', 'json')
        self._subway_wait = self.request.headers.get('X-Subway-Wait')
        self._locator = MockClientLocator.instance()

    @tornado.gen.coroutine
    def _check_service_ticket(self):
        return (200, "OK")

def make_mock_delivery_handler(port):
    class H(MockDeliveryHandler):
        def __init__(self, request, *args, **kwargs):
            kwargs.update(dict(subway_port=port))
            super(H, self).__init__(request, *args, **kwargs)
    return H

# ====================================================================================================================
class ServerContext:
    def __init__(self, port):
        self._log = logging.getLogger('delivery.test.context')

        self.subway = SubwayServer(port=port, nocache=True)

        self.pull = SubwayPullClient(port=port, nocache=True)

        self.delivery = DeliveryServer(
            port=7788,
            subway_port=port,
            handler_class=make_mock_delivery_handler(port),
            procs=1
        )
        utils.GlobalState.set_listening()
        utils.GlobalState.set_ready()

        self.http = tornado.httpclient.AsyncHTTPClient()

        self.pull.start(False)

    # ----------------------------------------------------------------------------------------------------------------
    def __enter__(self):
        self.subway.start()
        self.delivery.start()
        return self

    # ----------------------------------------------------------------------------------------------------------------
    def __exit__(self, *args, **kwargs):
        self.subway.stop()
        self.delivery.stop()
        assert self.pull is None

    # ----------------------------------------------------------------------------------------------------------------
    def make_fanout_request(self, guids, legacy=True):

        message = TOutMessage()
        message.PayloadId = '42'
        for guid in guids:
            message.Guids.append(guid)

        data = message.SerializeToString()
        header = struct.pack('<IQ', 2, CityHash64(data))

        request = tornado.httpclient.HTTPRequest(
            'http://localhost:7788/delivery',
            method='POST',
            headers={
                'Content-Type':     'application/octet-stream',
                'X-Subway-Format':  'json' if legacy else 'proto',
                'X-Ya-Service-Ticket': 'xyu',
            },
            body=header + data
        )

        return request

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def post_delivery(self, guids, legacy=True):
        try:
            request = self.make_fanout_request(guids, legacy)
            response = yield self.http.fetch(request)
        except Exception as ex:
            self._log.exception(ex)
            raise

# ====================================================================================================================
class UnisystemContext:
    def __init__(self, count=5, client_id_based=True):
        self._systems = [
            MockUnisystem() for i in range(0, count)
        ]

        locator = MockClientLocator().instance()
        locator.reset()
        for s in self._systems:
            if client_id_based:
                locator.add_client_id(s.guid, s.session_id)
            else:
                locator.add_uuid(s.guid, s.uuid())

    # ----------------------------------------------------------------------------------------------------------------
    def get(self, n):
        system = self._systems[n]
        return system.guid, system.uuid(), system.session_id


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def _test_locator_mock():
    guid = str(uuid_.uuid4())
    uuid = str(uuid_.uuid4())
    client_id = str(uuid_.uuid4())

    locator = MockClientLocator()
    locator.add_uuid(guid, uuid)
    locator.add_client_id(guid, client_id)

    locs = yield locator.resolve_locations([guid])
    for g, entry in locs.items():
        assert g == guid
        for u, c, l, n in entry.enumerate_locations():
            if u is not None:
                assert u == uuid

            if c is not None:
                assert c == client_id

            assert l == 'localhost'

    locs = yield locator.resolve_locations([str(uuid_.uuid4())])
    assert len(locs) == 0


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_delivery_single_message():
    ucx = UnisystemContext(client_id_based=True)

    with ServerContext(port=9901) as c:
        try:
            yield tornado.gen.sleep(0.1)

            guid, uuid, client_id = ucx.get(0)

            yield [
                c.pull.add_client(s) for s in ucx._systems
            ]

            yield c.post_delivery([guid], legacy=False)

            ok, messages = yield c.pull.pull(timeout=1.0)
            assert ok
            assert len(messages) == 1
            assert len(messages[0].Destinations) == 1
            assert messages[0].Message.PayloadId == '42'
        finally:
            yield c.pull.stop()
            c.pull = None

    yield tornado.gen.sleep(0.1)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_delivery_multi_message():
    ucx = UnisystemContext(client_id_based=True)

    with ServerContext(port=9900) as c:
        try:
            yield tornado.gen.sleep(0.1)

            guid, uuid, client_id = ucx.get(0)

            yield [
                c.pull.add_client(s) for s in ucx._systems
            ]

            yield c.post_delivery(list([s.guid for s in ucx._systems]), legacy=False)

            yield tornado.gen.sleep(0.5)

            ok, messages = yield c.pull.pull(timeout=1.0)
            logging.info('MULTI message %s', messages)
            assert ok
            assert len(messages) == 1
            assert len(messages[0].Destinations) == 5
            assert messages[0].Message.PayloadId == '42'
        finally:
            yield c.pull.stop()
            c.pull = None

    yield tornado.gen.sleep(0.1)


# ====================================================================================================================
@alice.uniproxy.library.testing.ioloop_run
def test_delivery_missing_sessions():
    extra_clients_cnt = 10
    clients_cnt = g_locations_remover.MAX_REMOVING_SIMULTANEOUSLY + extra_clients_cnt
    ucx = UnisystemContext(count=clients_cnt, client_id_based=True)
    multiply_message = 3

    with ServerContext(port=9989) as c:
        try:
            yield tornado.gen.sleep(0.1)

            guid, uuid, client_id = ucx.get(0)

            yield [
                c.pull.add_client(s) for s in ucx._systems
            ]

            run_remove_cnt0 = g_locations_remover.counter_run_remove()
            skip_remove_cnt0 = g_locations_remover.counter_skip_remove()

            g_locations_remover.test_mode(True)
            # remove recipients from subway

            yield [
                c.pull.remove_client(s) for s in ucx._systems
            ]

            logging.debug('POST_DELIVERY ' + '#' * 80)
            for i in range(multiply_message):
                yield c.post_delivery(list([s.guid for s in ucx._systems]), legacy=False)

            assert len(g_locations_remover._removing_locations) == g_locations_remover.MAX_REMOVING_SIMULTANEOUSLY
        finally:
            yield c.pull.stop()
            c.pull = None

    # with test_mode=True removing location work slow (need for reach MAX_REMOVING_SIMULTANEOUSLY)
    yield tornado.gen.sleep(10)
    g_locations_remover.test_mode(False)
    # check finish all removings
    assert len(g_locations_remover._removing_locations) == 0
    # check delivery remove as much missing recipients from ClientLocator as can
    assert g_locations_remover.counter_run_remove() - run_remove_cnt0 == g_locations_remover.MAX_REMOVING_SIMULTANEOUSLY
    assert g_locations_remover.counter_skip_remove()/multiply_message - skip_remove_cnt0 == extra_clients_cnt
    # check cache removed locations contain last deleted
    assert len(g_locations_remover._last_removed_locations) >= g_locations_remover.MAX_REMOVING_SIMULTANEOUSLY
    # give time for purge cache
    yield tornado.gen.sleep(2 * g_locations_remover.KEEPALIVE_LAST_REMOVED_LOCATIONS_INFO + 2)
    # check cache removed locations is clear
    assert len(g_locations_remover._last_removed_locations) == 0
