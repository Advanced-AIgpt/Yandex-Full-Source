import tornado.gen
import tornado.ioloop

from alice.uniproxy.library.unimemcached.pool import ConnectionPool


class ConnectionStub(object):
    def __init__(self, *args, **kwargs):
        super(ConnectionStub, self).__init__()

    def connect(self):
        pass

    def is_active(self):
        return True

    @tornado.gen.coroutine
    def set(self, key, value):
        pass

    @tornado.gen.coroutine
    def get(self, *keys):
        if not keys:
            raise tornado.gen.Return(None)

        if len(keys) >= 1:
            raise tornado.gen.Return({})

        raise tornado.gen.Return(None)


def test_connection_pool():
    @tornado.gen.coroutine
    def main():
        pool = ConnectionPool('localhost', connection_class=ConnectionStub, pool_size=3)
        yield pool.prepare_connections()

        conns = yield [pool.acquire() for i in range(0, 3)]
        assert(all(conns))
        yield [pool.release(conn) for conn in conns]

    tornado.ioloop.IOLoop().instance().run_sync(main)
