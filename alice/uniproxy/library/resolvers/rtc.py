import json
import tornado.gen
import tornado.ioloop
import tornado.concurrent
import tornado.httpclient

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config


RTC_DISCOVERY_PERIOD = config.get('delivery', {}).get('rtc_resolver', {}).get('period', 20.0)
RTC_UNIPROXY_SERVICES = config.get('delivery', {}).get('rtc_resolver', {}).get('uniproxy-services', {'uniproxy_sas': 'sas', 'uniproxy_man': 'man', 'uniproxy_vla': 'vla'})


g_rtc_resolver_instance = None


# ====================================================================================================================
class RTCResolver(object):
    def __init__(self, custom_period=None):
        super().__init__()
        self._log = Logger.get('delivery.dns')
        self._resolve_stop_request = False
        self._services = RTC_UNIPROXY_SERVICES
        self._resolve_futures = {}
        self._uniproxy_hostnames = {}
        self._uniproxy_all_hostnames = set()
        self._period = custom_period if custom_period else RTC_DISCOVERY_PERIOD
        self._client = None
        for service in self._services:
            self._resolve_futures[service] = tornado.concurrent.Future()
            self._uniproxy_hostnames[service] = set()

    # ----------------------------------------------------------------------------------------------------------------
    def _set_client(self):
        if not self._client:
            self._client = tornado.httpclient.AsyncHTTPClient(force_instance=True)

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        self._set_client()
        for service, cluster in self._services.items():
            tornado.ioloop.IOLoop.current().spawn_callback(self._resolve_main, service, cluster)

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._resolve_stop_request = True

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def resolve_uniproxy(self, hostname):
        result = hostname if hostname in self._uniproxy_all_hostnames else None

        if result is not None:
            return result

        for service in self._services:
            if not self._resolve_futures[service].done():
                yield self._resolve_futures[service]

        result = hostname if hostname in self._uniproxy_all_hostnames else None

        return result

    # ----------------------------------------------------------------------------------------------------------------
    @staticmethod
    def instance():
        global g_rtc_resolver_instance
        if g_rtc_resolver_instance is None:
            g_rtc_resolver_instance = RTCResolver()
        return g_rtc_resolver_instance

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _resolve_main(self, service, cluster):
        self._log.info('_resolve_main')
        while not self._resolve_stop_request:
            try:
                self._log.info('making request...')
                request = tornado.httpclient.HTTPRequest(
                    'https://hq.{}-swat.yandex-team.ru/rpc/instances/FindInstances/'.format(cluster),
                    method='POST',
                    headers={
                        'Content-Type': 'application/json',
                    },
                    body=json.dumps({
                        'filter': {
                            'serviceId': service
                        }
                    }),
                    validate_cert=False
                )
                response = yield self._client.fetch(request)
                if not response:
                    raise Exception('Got empty response')
                self._log.info('got response {}'.format(response))

                self._process_response(response, service)

                if not self._resolve_futures[service].done():
                    self._resolve_futures[service].set_result(True)
            except Exception as ex:
                self._log.error(ex)
            finally:
                yield tornado.gen.sleep(self._period)
        self._client.close()

    # ----------------------------------------------------------------------------------------------------------------
    def _process_response(self, response, service):
        self._log.info('processing response')
        try:
            data = json.loads(response.body.decode('utf-8'))
            if 'instance' not in data:
                self._log.error('no instances in /FindInstances response')
                return

            data_instances = data.get('instance', [])
            new_service_hostnames = set()
            for inst in data_instances:
                inst_hostname = inst['spec']['hostname']
                new_service_hostnames.add(inst_hostname)

            new_hostnames = (self._uniproxy_all_hostnames - self._uniproxy_hostnames[service]) | new_service_hostnames
            self._uniproxy_all_hostnames = new_hostnames
            self._uniproxy_hostnames[service] = new_service_hostnames
        except Exception as ex:
            self._log.error(ex)
