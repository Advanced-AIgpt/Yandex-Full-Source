import tornado.gen
import tornado.ioloop
import uuid

from alice.uniproxy.library.unimemcached.client import Client


TEST_UUIDS_COUNT = 1000

g_cache = {}


# ====================================================================================================================
class ConnectionStub(object):
    def __init__(self, host, **kwargs):
        super(ConnectionStub, self).__init__()
        global g_cache

        self.host = host
        if host not in g_cache:
            g_cache[self.host] = {}

    def connect(self):
        pass

    def is_active(self):
        return True

    @tornado.gen.coroutine
    def set(self, key, value, **kwargs):
        global g_cache
        g_cache[self.host][key] = value
        raise tornado.gen.Return(True)

    @tornado.gen.coroutine
    def get(self, *keys, **kwargs):
        as_dict = kwargs.get('as_dict', False)
        global g_cache

        if not keys:
            raise tornado.gen.Return(None)

        if len(keys) > 1:
            r = {key: g_cache[self.host].get(key) for key in keys if g_cache[self.host].get(key) is not None}
            raise tornado.gen.Return(r)
        elif len(keys) == 1:
            key = keys[0]
            if as_dict:
                raise tornado.gen.Return({key: g_cache[self.host].get(key)})
            else:
                raise tornado.gen.Return(g_cache[self.host].get(key))

        raise tornado.gen.Return(None)


def test_client():
    @tornado.gen.coroutine
    def main():
        client = Client(['test1:80', 'test2:81', 'test3:82'], connection_class=ConnectionStub)
        yield client.connect()

        keys = list([k for k in 'uniproxy'])
        for k in keys:
            yield client.xset(k, k)

        # test getting values
        r = yield client.xget_multi(*keys)
        assert(len(r) == len('uniproxy'))

        r = yield client.xget_multi(*keys)
        assert(len(r) == len('uniproxy'))

    tornado.ioloop.IOLoop().instance().run_sync(main)


def test_client_get():
    @tornado.gen.coroutine
    def main():
        client = Client(['test1:80', 'test2:81', 'test3:82'], connection_class=ConnectionStub)
        yield client.connect()

        result = yield client.set('foo', 'value')
        assert(result)

        result = yield client.get('foo', as_dict=False)
        assert(type(result) == str)

        result = yield client.get('foo', as_dict=True)
        assert(type(result) == dict)
        assert(result.get('foo') == 'value')

    tornado.ioloop.IOLoop().instance().run_sync(main)


@tornado.gen.coroutine
def one_server_lost():
    global g_cache
    g_cache = {}

    client = Client(['test:80', 'test:81', 'test:82'], connection_class=ConnectionStub)
    yield client.connect()

    keys = [str(uuid.uuid1()) for i in range(0, TEST_UUIDS_COUNT)]
    for k in keys:
        client.xset(k, k)

    g_cache['test:80'] = {}

    r = yield client.xget_multi(*keys)
    r = {r[x] for x in r.keys() if r[x] is not None}
    assert(len(r) == len(keys))


@tornado.gen.coroutine
def two_server_lost():
    global g_cache
    g_cache = {}

    client = Client(['test:80', 'test:81', 'test:82'], connection_class=ConnectionStub)
    yield client.connect()

    keys = [str(uuid.uuid1()) for i in range(0, TEST_UUIDS_COUNT)]
    for k in keys:
        client.xset(k, k)

    g_cache['test:80'] = {}
    g_cache['test:82'] = {}

    r = yield client.xget_multi(*keys)
    assert(len(r) >= 0.6 * len(keys))


def test_one_server_lost():
    tornado.ioloop.IOLoop().instance().run_sync(one_server_lost)


def test_two_server_lost():
    tornado.ioloop.IOLoop().instance().run_sync(one_server_lost)
