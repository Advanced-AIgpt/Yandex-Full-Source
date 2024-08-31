import tornado.gen
import alice.uniproxy.library.testing

from alice.uniproxy.library.unimemcached.client_mock import ClientMock


@alice.uniproxy.library.testing.ioloop_run
def test_mock_set():
    client = ClientMock(['first.yandex.net:80', 'second.yandex.net:80', 'third.yandex.net:80'])
    yield client.connect()

    ret = yield client.set('foo', '10', 1)
    assert ret

    result = yield client.get('foo')
    assert len(result) == 1

    ret = yield client.set('foo', '10', 1)
    assert ret

    yield tornado.gen.sleep(1.5)

    result = yield client.get('foo')
    assert not result

    ret = yield client.set('foo', '10')
    assert ret

    yield tornado.gen.sleep(10)

    result = yield client.get('foo')
    assert result


@alice.uniproxy.library.testing.ioloop_run
def test_mock_delete():
    client = ClientMock(['first.yandex.net:80', 'second.yandex.net:80', 'third.yandex.net:80'])
    yield client.connect()

    ret = yield client.set('dd', '10')
    assert ret

    result = yield client.get('dd')
    assert len(result) == 1

    ret = yield client.delete('dd')
    assert ret

    result = yield client.get('dd')
    assert not result


@alice.uniproxy.library.testing.ioloop_run
def test_mock_add():
    client = ClientMock(['first.yandex.net:80', 'second.yandex.net:80', 'third.yandex.net:80'])
    yield client.connect()

    ret = yield client.add('zzz', '10', 1)
    assert ret

    result = yield client.get('zzz')
    assert len(result) == 1

    ret = yield client.add('zzz', '10')
    assert not ret

    yield tornado.gen.sleep(1.2)

    result = yield client.get('zzz')
    assert not result


@alice.uniproxy.library.testing.ioloop_run
def test_mock_cas():
    client = ClientMock(['first.yandex.net:80', 'second.yandex.net:80', 'third.yandex.net:80'])
    yield client.connect()

    ret = yield client.add('ccc', '10', 10)
    assert ret

    result = yield client.get('ccc')
    assert len(result) == 1

    value, cas = yield client.gets('ccc')
    assert value
    assert cas

    ok, found, casok = yield client.cas('ccc', '11', '0')
    assert ok
    assert found
    assert not casok

    ok, found, casok = yield client.cas('ccc', '11', cas, 1)
    assert ok
    assert found
    assert casok

    yield tornado.gen.sleep(1.2)
    value, cas = yield client.gets('ccc')
    assert not value
    assert not cas
