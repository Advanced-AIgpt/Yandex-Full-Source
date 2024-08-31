import tornado.ioloop
import tornado.queues
import tornado.iostream
import tornado.tcpclient
import ketama

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.unimemcached.connection import Connection as DefaultConnection
from alice.uniproxy.library.unimemcached.pool import ConnectionPool
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_POOL_SIZE
from alice.uniproxy.library.unimemcached.const import UMEMCACHED_DEFAULT_RECONNECT_PERIOD


# ====================================================================================================================
class Client(object):
    def __init__(self, hosts, pool_size=UMEMCACHED_DEFAULT_POOL_SIZE, reconnect_period=UMEMCACHED_DEFAULT_RECONNECT_PERIOD, ioloop=None, **kwargs):
        super(Client, self).__init__()
        self._ioloop = tornado.ioloop.IOLoop.current() if ioloop is None else ioloop
        self.log = Logger.get('memcached')
        self.hosts = self.apply_aliases(hosts, kwargs.get('aliases', {}))
        self.pool_size = pool_size
        self.reconnect_period = reconnect_period

        self.connection_class = kwargs.get('connection_class', DefaultConnection)

        self.pools_count = len(self.hosts)
        self.continuum = ketama.Continuum([(hosts[i][:36], i, 1) for i in range(0, len(hosts))])

        self._performance_callback = kwargs.get(
            'performance_callback',
            self._default_performance_callback
        )

        self._server_restarted_callback = kwargs.get(
            'server_restarted_callback',
            self._default_server_restarted_callback
        )

        self.pools = [
            ConnectionPool(
                host,
                pool_size=self.pool_size,
                connection_class=self.connection_class,
                reconnect_period=self.reconnect_period,
                server_restarted_callback=self._server_restarted_callback
            )
            for host in self.hosts
        ]

    # ----------------------------------------------------------------------------------------------------------------
    def apply_aliases(self, hosts, aliases):
        return [aliases.get(host, host) for host in hosts]

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _default_performance_callback(self, command, time):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _default_server_restarted_callback(self, host, index):
        self.log.warning('server restart detected, but no callback is set: index={} host={}'.format(index, host))

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def connect(self):
        futures = [pool.prepare_connections() for pool in self.pools]
        yield futures

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def make_constant(self, value):
        return value

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xset(self, key, value, exptime=0):
        try:
            if (key is None) or (value is None):
                return None

            index, yndex = self.servers_for_key(key)

            ipool = self.pools[index]
            ypool = self.pools[yndex]

            iconn, yconn = yield [
                ipool.acquire(),
                ypool.acquire()
            ]

            ires, yres = yield [
                iconn.set(key, value, exptime=exptime) if iconn else self.make_constant(False),
                yconn.set(key, value, exptime=exptime) if yconn else self.make_constant(False),
            ]

            yield [
                ipool.release(iconn) if iconn else self.make_constant(None),
                ypool.release(yconn) if yconn else self.make_constant(None)
            ]

            return ires or yres
        except Exception as ex:
            self.log.warning('xset/error: {}'.format(ex))

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def set(self, key, value, exptime=0):
        if (key is None) or (value is None):
            return False

        index = self.server_for_key(key)
        ipool = self.pools[index]
        iconn = yield ipool.acquire()
        ires = (yield iconn.set(key, value, exptime=exptime)) if iconn else None

        if iconn:
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def incr(self, key, value, server=-1):
        if (key is None) or (not isinstance(value, int)) or (value < 0):
            return None

        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]

        iconn = yield ipool.acquire()

        ires = None
        if iconn:
            ires = yield iconn.incr(key, value)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def decr(self, key, value, server=-1):
        if (key is None) or (not isinstance(value, int)) or (value < 0):
            return None

        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]

        iconn = yield ipool.acquire()

        ires = None
        if iconn:
            ires = yield iconn.decr(key, value)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def add(self, key, value, exptime=0, server=-1):
        if (key is None) or (value is None):
            return None

        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]

        iconn = yield ipool.acquire()

        ires = None
        if iconn:
            ires = yield iconn.add(key, value, exptime=exptime)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xadd(self, key, value, exptime=0):
        try:
            if (key is None) or (value is None):
                return None

            index, yndex = self.servers_for_key(key)

            ipool = self.pools[index]
            ypool = self.pools[yndex]

            iconn, yconn = yield [
                ipool.acquire(),
                ypool.acquire()
            ]

            ires, yres = yield [
                iconn.add(key, value, exptime=exptime) if iconn else self.make_constant(False),
                yconn.add(key, value, exptime=exptime) if yconn else self.make_constant(False),
            ]

            yield [
                ipool.release(iconn) if iconn else self.make_constant(None),
                ypool.release(yconn) if yconn else self.make_constant(None)
            ]

            return ires or yres
        except Exception as ex:
            self.log.warning('xadd/error: {}'.format(ex))

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def delete(self, key, server=-1):
        if (key is None):
            return None

        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]
        iconn = yield ipool.acquire()

        ires = None
        if iconn:

            ires = yield iconn.delete(key)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xdelete(self, key):
        results = []

        for server in self.servers_for_key(key):
            pool = self.pools[server]
            conn = yield pool.acquire()

            if conn:
                try:
                    r = yield conn.delete(key)
                    results.append(r)
                finally:
                    yield pool.release(conn)

        return all(results)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xset_multi(self, _dict):
        for server, keys in self.enumerate_hosts_for_keys(_dict.keys()):
            pool = self.pools[server]
            conn = yield pool.acquire()

            if conn:
                try:
                    for key in keys:
                        yield conn.set(key, _dict[key])
                finally:
                    yield pool.release(conn)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def get(self, key, server=-1, as_dict=False, binary=False):
        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]

        iconn = yield ipool.acquire()

        ires = None
        if iconn:
            ires = yield iconn.get(key, as_dict=as_dict, binary=binary)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def gets(self, key, server=-1, binary=False):
        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]
        iconn = yield ipool.acquire()

        value, cas_unique = (None, None)
        if iconn:
            value, cas_unique = yield iconn.gets(key, binary=binary)
            yield ipool.release(iconn)

        return (value, cas_unique)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def cas(self, key, value, cas_unique, exptime=0, server=-1):
        if server < 0:
            server = self.server_for_key(key)

        ipool = self.pools[server]
        iconn = yield ipool.acquire()

        ok, found, cas = (False, False, None)
        if iconn:
            ok, found, cas = yield iconn.cas(key, value, cas_unique, exptime=exptime)
            yield ipool.release(iconn)

        return (ok, found, cas)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def get_multi(self, *keys, server=-1, binary=False):
        if len(keys) == 0:
            return {}

        if server < 0:
            server = self.server_for_key(keys[0])

        ipool = self.pools[server]

        iconn = yield ipool.acquire()

        ires = None
        if iconn:
            ires = yield iconn.get(*keys, as_dict=True, binary=binary)
            yield ipool.release(iconn)

        return ires

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xget(self, key, binary=False):
        i, j = self.servers_for_key(key)

        value = None

        check_second_server = False
        try:
            value = yield self.get(key, server=i, binary=binary)
            if not value:
                check_second_server = True
        except:
            check_second_server = True

        if not check_second_server:
            return value

        try:
            value = yield self.get(key, server=j, binary=binary)
        except:
            value = None

        return value

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def xget_multi(self, *keys, binary=False):
        data = {}
        for server, _keys in self.enumerate_hosts_for_keys(keys):
            result = yield self.get_multi(*_keys, server=server, binary=binary)

            if result:
                data.update(result)

        return data

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def stats(self):
        data = {}

        for server in range(0, len(self.pools)):
            pool = self.pools[server]

            try:
                conn = yield pool.acquire()

                if conn:
                    try:
                        result = yield conn.stats()
                        data[pool.host] = result
                    except:
                        data[pool.host] = {}

            finally:
                yield pool.release(conn)

        return data

    # ----------------------------------------------------------------------------------------------------------------
    def server_for_key(self, key):
        return self.continuum[key]

    # ----------------------------------------------------------------------------------------------------------------
    def servers_for_key(self, key):
        s1 = self.continuum[key]
        s2 = (s1 + 1) % self.pools_count
        return s1, s2

    # ----------------------------------------------------------------------------------------------------------------
    def enumerate_hosts_for_keys(self, keys):
        kvpairs = []

        for servers, key in [(self.servers_for_key(k), k) for k in keys]:
            for s in servers:
                kvpairs.append((s, key))

        kvpairs = sorted(kvpairs)

        if not len(kvpairs):
            return

        server = kvpairs[0][0]
        _keys = []
        for s, k in kvpairs:
            if s == server:
                _keys.append(k)
            else:
                yield server, _keys
                _keys = [k]
                server = s

        yield server, _keys
