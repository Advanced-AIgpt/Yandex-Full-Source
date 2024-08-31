import tornado.gen
import tornado.ioloop
import tornado.concurrent

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPError, HTTPRequest, HTTPResponse
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


QLOUD_DISCOVERY_PERIOD = config.get('delivery', {}).get('qloud_resolver', {}).get('period', 60.0)
QLOUD_DISCOVERY_UNIPROXY_COMPONENT = config.get('delivery', {}).get('qloud_resolver', {}).get('uniproxy-component')


g_qloud_resolver_instance = None


# ====================================================================================================================
class QloudResolver(object):
    def __init__(self):
        super().__init__()
        self._log = Logger.get('delivery.dns')
        self._resolve_init = False
        self._resolve_future = tornado.concurrent.Future()
        self._uniproxy_ips = {}
        self._resolve_stop_request = False

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        tornado.ioloop.IOLoop.current().spawn_callback(self._resolve_main)

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._resolve_stop_request = True

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def resolve_uniproxy(self, hostId):
        ipaddr = self._uniproxy_ips.get(hostId)

        if ipaddr is not None:
            return ipaddr

        if not self._resolve_init:
            yield self._resolve_future

        ipaddr = self._uniproxy_ips.get(hostId)

        return ipaddr

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def instance():
        global g_qloud_resolver_instance
        if g_qloud_resolver_instance is None:
            g_qloud_resolver_instance = QloudResolver()
        return g_qloud_resolver_instance

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _resolve_main(self):
        self._log.info('_resolve_main')
        client = QueuedHTTPClient('localhost', 1, pool_size=3)
        query = '/metadata?component={}&status=current'.format(QLOUD_DISCOVERY_UNIPROXY_COMPONENT)

        while not self._resolve_stop_request:
            try:
                self._log.info('making request...')
                request = HTTPRequest(
                    query,
                    method='GET',
                    headers={
                        'Host': 'localhost:1',
                    }
                )

                self._log.info('fetching request...')
                response = yield client.fetch(request)
                self._log.info('got response {}'.format(response))

                self._process_response(response)

                if not self._resolve_init:
                    self._resolve_init = True
                    self._resolve_future.set_result(True)
            except HTTPError as ex:
                self._log.error(ex)
            finally:
                yield tornado.gen.sleep(QLOUD_DISCOVERY_PERIOD)

    # ----------------------------------------------------------------------------------------------------------------
    def _process_response(self, response: HTTPResponse):
        self._log.info('processing response')
        try:
            data = response.json()
            if 'instances' not in data:
                self._log.error('no instances in /metainfo response')
                return

            addresses = {}
            for inst in data['instances']:
                inst_id = inst['id']
                inst_ip = inst['ip']
                addresses[inst_id] = inst_ip

            self._uniproxy_ips.update(addresses)
        except Exception as ex:
            self._log.error(ex)
