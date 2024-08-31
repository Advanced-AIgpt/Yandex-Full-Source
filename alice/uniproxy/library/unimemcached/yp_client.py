import tornado.gen
import tornado.ioloop

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.unimemcached.client import Client


class YpClient(object):
    def __init__(self, podset_id, resolver, pool_size=3):
        self.client = None
        self.podset_id = podset_id
        self.pool_size = pool_size
        self.ready = False
        self.resolver = resolver
        self._log = Logger.get('memcached.yp')
        tornado.ioloop.IOLoop.current().spawn_callback(self.updater)

    @tornado.gen.coroutine
    def updater(self):
        instanses = set()
        while True:
            try:
                self._log.info('trying to update')
                new_instanses = set((yield self.resolver.resolve_podset(self.podset_id)).values())

                if instanses.symmetric_difference(new_instanses) or not self.ready:
                    instanses = new_instanses
                    hosts = list(instanses)
                    new_client = Client(hosts, pool_size=self.pool_size)
                    yield new_client.connect()
                    self.client = new_client
                    self._log.info('client updated')
                    self.ready = True
            except Exception as ex:
                self._log.info('Exception in updater: {}'.format(ex))
            finally:
                yield tornado.gen.sleep(30)

    def apply_aliases(self, hosts, aliases):
        if not self.ready:
            return
        return self.client.apply_aliases(hosts, aliases)

    @tornado.gen.coroutine
    def connect(self):
        if not self.ready:
            return
        yield self.client.connect()

    @tornado.gen.coroutine
    def xset(self, key, value, exptime=0):
        if not self.ready:
            return
        res = yield self.client.xset(key, value, exptime)
        return res

    @tornado.gen.coroutine
    def set(self, key, value, exptime=0):
        if not self.ready:
            return
        res = yield self.client.set(key, value, exptime)
        return res

    @tornado.gen.coroutine
    def incr(self, key, value, server=-1):
        if not self.ready:
            return
        res = yield self.client.incr(key, value, server)
        return res

    @tornado.gen.coroutine
    def decr(self, key, value, server=-1):
        if not self.ready:
            return
        res = yield self.client.decr(key, value, server)
        return res

    @tornado.gen.coroutine
    def add(self, key, value, exptime=0, server=-1):
        if not self.ready:
            return
        res = yield self.client.add(key, value, exptime, server)
        return res

    @tornado.gen.coroutine
    def xadd(self, key, value, exptime=0):
        if not self.ready:
            return
        res = yield self.client.xadd(key, value, exptime)
        return res

    @tornado.gen.coroutine
    def delete(self, key, server=-1):
        if not self.ready:
            return
        res = yield self.client.delete(key, server)
        return res

    @tornado.gen.coroutine
    def xdelete(self, key):
        if not self.ready:
            return
        res = yield self.client.xdelete(key)
        return res

    @tornado.gen.coroutine
    def xset_multi(self, _dict):
        if not self.ready:
            return
        yield self.client.xset_multi(_dict)

    @tornado.gen.coroutine
    def get(self, key, server=-1, as_dict=False, binary=False):
        if not self.ready:
            return
        res = yield self.client.get(key, server, as_dict, binary)
        return res

    @tornado.gen.coroutine
    def gets(self, key, server=-1, binary=False):
        if not self.ready:
            return
        res = yield self.client.gets(key, server, binary)
        return res

    @tornado.gen.coroutine
    def cas(self, key, value, cas_unique, exptime=0, server=-1):
        if not self.ready:
            return
        res = yield self.client.cas(key, value, cas_unique, exptime, server)
        return res

    @tornado.gen.coroutine
    def get_multi(self, *keys, server=-1, binary=False):
        if not self.ready:
            return
        res = yield self.client.get_multi(*keys, server, binary)
        return res

    @tornado.gen.coroutine
    def xget(self, key, binary=False):
        if not self.ready:
            return
        res = yield self.client.xget(key, binary)
        return res

    @tornado.gen.coroutine
    def xget_multi(self, *keys, binary=False):
        if not self.ready:
            return
        res = yield self.client.xget_multi(*keys, binary)
        return res

    @tornado.gen.coroutine
    def stats(self):
        if not self.ready:
            return
        res = yield self.client.stats()
        return res

    def server_for_key(self, key):
        if not self.ready:
            return
        return self.client.server_for_key(key)

    def servers_for_key(self, key):
        if not self.ready:
            return
        return self.client.servers_for_key(key)

    def enumerate_hosts_for_keys(self, keys):
        if not self.ready:
            return
        self.client.enumerate_hosts_for_keys(keys)
