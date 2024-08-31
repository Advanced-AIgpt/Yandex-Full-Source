import os
import tornado.gen
import tornado.ioloop

from .rtcdiscovery import RtcDiscovery

import alice.uniproxy.library.unimemcached as unimemcached
from alice.uniproxy.library.logging import Logger
# from alice.uniproxy.library.resolvers.yp import YpResolver
YpResolver = None

MEMCACHED_DEFAULT_EXPIRE_TIME = 43200   # 43200 seconds == 12 hours
MEMCACHED_DEFAULT_POOL_SIZE = 16


# ====================================================================================================================
class ClientProvider(object):
    def __init__(self, fake=False):
        super(ClientProvider, self).__init__()
        self._log = Logger.get('memcached.provider')
        self.ClientClass = unimemcached.Client if not fake else unimemcached.ClientMock
        self._clients = {}

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def init_with_hosts(self, name, hosts, pool_size, ttl, aliases={}):
        try:
            self._log.debug('init {} with hosts: {}'.format(name, hosts))
            client = self.ClientClass(hosts, pool_size=pool_size, aliases=aliases)

            self._log.debug('init {} with hosts: connecting...'.format(name))
            yield client.connect()
            self._log.debug('init {} with hosts: connecting done'.format(name))

            self._clients[name] = client
        except Exception as ex:
            self._log.exception(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def init_with_services(self, name, services, pool_size, ttl, cross_dc):
        self._log.debug('init {} with services: {}'.format(name, services))

        discoveryClient = RtcDiscovery(default_location='sas', cross_dc=cross_dc)

        hosts = yield discoveryClient.get_memcached_insances(memcached_id=name)

        self._log.info('init {} with services: {}'.format(name, hosts))
        if hosts:
            client = self.ClientClass(hosts, pool_size=pool_size)
            yield client.connect()
            self._clients[name] = client

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def init_with_endpointsets(self, name, endpointset_ids, pool_size, ttl, cross_dc):
        self._log.debug('init {} with endpointsets: {}'.format(name, endpointset_ids))

        for podset_id in endpointset_ids:
            client = unimemcached.YpClient(podset_id, YpResolver.instance(), pool_size=pool_size)
            self._clients[name] = client

    # ----------------------------------------------------------------------------------------------------------------
    def get_geo(self):
        default_location = 'sas'
        rtc_bsconfig_tags = os.environ.get('BSCONFIG_ITAGS', '').split()
        for tag in rtc_bsconfig_tags:
            if tag.startswith('a_geo_'):
                self._log.info('current location tag is "{}"'.format(tag))
                return tag[6:].lower()
        self._log.debug('no location tag found, returning default ({})'.format(default_location))
        return default_location.lower()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def init(self, name, config):
        self._log.debug('init "{}"...'.format(name))

        service_ids = config.get('service_id', [])
        if isinstance(service_ids, str):
            service_ids = [service_ids]

        hosts = config.get('hosts', [])
        if isinstance(hosts, str):
            hosts = [hosts]

        endpointset_prefix = config.get('endpointset_prefix')

        endpointset_ids = []
        if endpointset_prefix:
            endpointset_ids.append(endpointset_prefix + self.get_geo())

        aliases = config.get('aliases', {})

        pool_size = config.get('pool_size', MEMCACHED_DEFAULT_POOL_SIZE)
        expire_time = config.get('expire_time', MEMCACHED_DEFAULT_EXPIRE_TIME)
        cross_dc = config.get('cross_dc', True)

        if hosts:
            yield self.init_with_hosts(name, hosts, pool_size, expire_time, aliases=aliases)
        elif service_ids:
            yield self.init_with_services(name, service_ids, pool_size, expire_time, cross_dc)
        elif endpointset_ids:
            yield self.init_with_endpointsets(name, endpointset_ids, pool_size, expire_time, cross_dc)

    # ----------------------------------------------------------------------------------------------------------------
    def init_sync(self, name, config):

        @tornado.gen.coroutine
        def run_sync_wrapper():
            yield self.init(name, config)

        tornado.ioloop.IOLoop.current().run_sync(run_sync_wrapper)

    # ----------------------------------------------------------------------------------------------------------------
    def get(self, name):
        self._log.debug('get memcached client {}'.format(name))
        return self._clients.get(name)
