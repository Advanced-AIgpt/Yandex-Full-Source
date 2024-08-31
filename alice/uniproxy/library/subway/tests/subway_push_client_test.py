import time
import uuid

import tornado.gen

import alice.uniproxy.library.testing

from alice.uniproxy.library.subway.pull_client import PullClient
from alice.uniproxy.library.subway.push_client import push_message

from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage

from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings


from .mocks import with_subway
from .mocks import UnisystemMock


GlobalCounter.init()
GlobalTimings.init()


@with_subway
@alice.uniproxy.library.testing.ioloop_run
def test_push_to_one(client: PullClient):
    yield client.wait_for_ready(3.0)

    guid = 'dd3a2b0e-df7f-11e9-9b23-525400123456'
    mock = UnisystemMock(guid)

    yield client.add_client(mock)

    message = TSubwayMessage()
    message.MessengerMsg.PayloadId = 'push-me'
    dest = message.Destinations.add()
    dest.Guid = uuid.UUID(guid).bytes

    ok, message = yield push_message('localhost', message, port=client.port())
    assert ok

    result = yield tornado.gen.with_timeout(time.time() + 1.0, mock.message)
    assert 'PayloadId' in result
    assert result['PayloadId'] == 'push-me'


@with_subway
@alice.uniproxy.library.testing.ioloop_run
def test_push_to_many(client: PullClient):
    guids = [
        'd459f0e8-df82-11e9-8c6b-525400123456',
        'd6c69c50-df82-11e9-a4f9-525400123456',
        'd8c71750-df82-11e9-bb22-525400123456',
    ]

    mocks = [UnisystemMock(guid) for guid in guids]

    yield [
        client.add_client(mock) for mock in mocks
    ]

    message = TSubwayMessage()
    message.MessengerMsg.PayloadId = 'push-me'

    for guid in guids:
        dest = message.Destinations.add()
        dest.Guid = uuid.UUID(guid).bytes

    yield push_message('localhost', message, port=client.port())

    for mock in mocks:
        result = yield tornado.gen.with_timeout(time.time() + 4.0, mock.message)
        assert 'PayloadId' in result
        assert result['PayloadId'] == 'push-me'
