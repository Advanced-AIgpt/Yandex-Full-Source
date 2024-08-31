import json
import logging
import os
import tornado.gen
import tornado.ioloop
import tornado.concurrent

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPError
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config

try:
    from library.python.vault_client import instances as YAV
except:
    pass

YP_DISCOVERY_PERIOD = config.get('yp_resolver', {}).get('period', 60.0)
RETRIES_NUM = config.get('yp_resolver', {}).get('retries_num', 1)
REQUEST_TIMEOUT = config.get('yp_resolver', {}).get('request_timeout', 1)
PODSET_IDS = {}
DC = ['man', 'sas', 'vla']
for dc in DC:
    PODSET_IDS[config.get('memcached', {}).get('tts', {}).get('endpointset_prefix', 'alice-memcached-yp-test-') + dc] = dc
VAULT_KEY = config.get('yp_resolver', {}).get('vault_key', '')

g_yp_resolver_instance = None


class Resolver(object):
    def __init__(self):
        self.client = QueuedHTTPClient('sd.yandex.net', 8080, pool_size=3)
        if 'YP_TOKEN' in os.environ:
            self.token = os.environ['YP_TOKEN']
            return
        kwargs = {}
        yav = YAV.Production(**kwargs)
        secrets = yav.get_version(VAULT_KEY)['value']
        if not secrets:
            raise Exception('Vault is unavailable')
        if 'YP_TOKEN' not in secrets:
            raise Exception('Vault has no secret \'YP_TOKEN\'')
        self.token = secrets['YP_TOKEN']

    @tornado.gen.coroutine
    def resolve_endpoints(self, podset_id):
        request = HTTPRequest(
            query='/resolve_endpoints/json',
            method='POST',
            headers={
                'Accept': 'application/json',
                'Authorization': 'OAuth {}'.format(self.token)
            },
            body=json.dumps({
                'cluster_name': PODSET_IDS[podset_id],
                'endpoint_set_id': podset_id,
                'client_name': 'uniproxy_yp_resolver'
            }).encode('utf-8'),
            retries=RETRIES_NUM,
            request_timeout=REQUEST_TIMEOUT
        )
        response = yield self.client.fetch(request)
        return response.json()


# ====================================================================================================================
class YpResolver(object):
    def __init__(self, custom_resolver=None):
        super().__init__()
        self._log = Logger.get('delivery.dns')
        self._resolve_init = {}
        self._resolve_future = {}
        for podset_id in PODSET_IDS:
            self._resolve_init[podset_id] = False
            self._resolve_future[podset_id] = tornado.concurrent.Future()
        self._uniproxy_ips = {}
        self._resolve_stop_request = False
        self._resolver = custom_resolver if custom_resolver else Resolver()
        global g_yp_resolver_instance
        g_yp_resolver_instance = self

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        for podset_id in PODSET_IDS:
            tornado.ioloop.IOLoop.current().spawn_callback(self._resolve_main, podset_id)

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._resolve_stop_request = True

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def resolve_podset(self, podset_id):
        if podset_id not in PODSET_IDS:
            raise Exception('Unknown podset_id: {}'.format(podset_id))
        if len(self._uniproxy_ips.get(podset_id, {})):
            return self._uniproxy_ips[podset_id]

        if not self._resolve_init[podset_id]:
            res = yield self._resolve_future[podset_id]
        if res is not True:
            raise Exception(res)

        return self._uniproxy_ips[podset_id]

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def instance():
        global g_yp_resolver_instance
        if g_yp_resolver_instance is None:
            g_yp_resolver_instance = YpResolver()
            g_yp_resolver_instance.start()
        return g_yp_resolver_instance

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _resolve_main(self, podset_id):
        logging.info('_resolve_main' + ' with podset_id: ' + podset_id)
        while not self._resolve_stop_request:
            try:
                response = yield self._resolver.resolve_endpoints(podset_id)
                if response is None or len(response) == 0:
                    raise Exception('Empty response from resolver')
                self._log.info('got response %s', response)

                self._process_response(response, podset_id)
                if not self._resolve_init[podset_id]:
                    self._resolve_init[podset_id] = True
                    self._resolve_future[podset_id].set_result(True)
            except HTTPError as ex:
                if ex.code in [598, 599]:
                    GlobalCounter.increment_counter('sd', str(ex.code), 'err')
                else:
                    GlobalCounter.increment_error_code('sd', ex.code)
                if not self._resolve_init[podset_id]:
                    self._resolve_future[podset_id].set_result('Got {} error from resolver'.format(ex.code))
            except Exception as ex:
                if not self._resolve_init[podset_id]:
                    self._resolve_future[podset_id].set_result(str(ex))
            finally:
                yield tornado.gen.sleep(YP_DISCOVERY_PERIOD)

    # ----------------------------------------------------------------------------------------------------------------
    def _process_response(self, response, podset_id):
        self._log.info('processing response')
        addresses = {}
        for endpoint in response['endpoint_set']['endpoints']:
            logging.info(endpoint)
            host = endpoint['ip6_address'], endpoint['fqdn'], endpoint['port']
            logging.info("yp host: {}".format(host))
            addresses[endpoint['fqdn']] = '{}:{}'.format(endpoint['ip6_address'], endpoint['port'])
        if not self._uniproxy_ips.get(podset_id):
            self._uniproxy_ips[podset_id] = {}
        self._uniproxy_ips[podset_id].update(addresses)
