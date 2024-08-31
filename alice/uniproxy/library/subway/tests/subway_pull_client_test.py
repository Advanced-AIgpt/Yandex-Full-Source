import alice.uniproxy.library.testing

from alice.uniproxy.library.subway.pull_client import PullClient

from .mocks import with_subway
from .mocks import UnisystemMock


@with_subway
@alice.uniproxy.library.testing.ioloop_run
def test_pull_enumerate(client: PullClient):
    yield client.wait_for_ready(3.0)
    result = yield client.enumerate()

    assert isinstance(result, dict)
    assert len(result) == 0


@with_subway
@alice.uniproxy.library.testing.ioloop_run
def test_pull_add_client(client: PullClient):
    guid = '71629f9a-df7c-11e9-8b48-525400123456'
    mock = UnisystemMock(guid)

    yield client.wait_for_ready(3.0)

    ok, message = yield client.add_client(mock)
    assert ok

    result = yield client.enumerate()

    assert isinstance(result, dict)
    assert len(result) == 1
    assert guid in result
    assert result[guid] == 1

    ok, message = yield client.add_client(mock)
    assert ok

    result = yield client.enumerate()

    assert len(result) == 1
    assert guid in result
    assert result[guid] == 2


@with_subway
@alice.uniproxy.library.testing.ioloop_run
def test_pull_remove_client(client: PullClient):
    guid = '71629f9a-df7c-11e9-8b48-525400123456'
    mock = UnisystemMock(guid)

    yield client.wait_for_ready(3.0)

    ok, message = yield client.add_client(mock)
    assert ok

    ok, message = yield client.add_client(mock)
    assert ok

    result = yield client.enumerate()

    assert len(result) == 1
    assert guid in result
    assert result[guid] == 2

    ok, message = yield client.remove_client(mock)
    assert ok

    result = yield client.enumerate()
    assert len(result) == 1
    assert guid in result
    assert result[guid] == 1

    ok, message = yield client.remove_client(mock)
    assert ok

    result = yield client.enumerate()
    assert len(result) == 0
