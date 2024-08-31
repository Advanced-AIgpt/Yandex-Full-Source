import tornado.ioloop
import tornado.httpserver
import tornado.web
import tornado.gen

from alice.uniproxy.library.common_handlers import PingHandler
from alice.uniproxy.library.common_handlers import UnistatHandler
from alice.uniproxy.library.common_handlers import StartHookHandler
from alice.uniproxy.library.common_handlers import StopHookHandler

from alice.uniproxy.library.resolvers import QloudResolver, RTCResolver

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import QLOUD_HTTP_PORT
from alice.uniproxy.library.settings import UNIPROXY_MAX_FORKS
from alice.uniproxy.library.settings import config

from alice.uniproxy.library.delivery.handler import MessengerHandler


QLOUD_DISCOVERY_ENABLED = config.get('delivery', {}).get('qloud_resolver', {}).get('enabled', False)
RTC_DISCOVERY_ENABLED = config.get('delivery', {}).get('rtc_resolver', {}).get('enabled', False)

if QLOUD_DISCOVERY_ENABLED and RTC_DISCOVERY_ENABLED:
    raise Exception('More than one resolver enabled')


# ====================================================================================================================
class DeliveryServer(object):
    def __init__(self, *args, **kwargs):
        super(DeliveryServer, self).__init__()

        self._port = kwargs.get('port', QLOUD_HTTP_PORT)

        self._subway_port = kwargs.get('subway_port', config.get('subway', {}).get('port', 7777))

        self._procs = kwargs.get('procs', config.get('delivery', {}).get('procs', UNIPROXY_MAX_FORKS))

        self._log = Logger.get('delivery.server')

        default_handler = MessengerHandler
        self._delivery_handler = kwargs.get('handler_class', default_handler)

        if QLOUD_DISCOVERY_ENABLED:
            self._resolver = QloudResolver.instance()

        if RTC_DISCOVERY_ENABLED:
            self._resolver = RTCResolver.instance()

        self._app = tornado.web.Application([
            (r'/delivery', self._delivery_handler),
            (r'/ping', PingHandler),
            (r'/unistat', UnistatHandler),
            (r"/stop_hook", StopHookHandler),
            (r"/start_hook", StartHookHandler),
        ])

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(self._port)
        self._srv.start(self._procs)

        if QLOUD_DISCOVERY_ENABLED or RTC_DISCOVERY_ENABLED:
            self._resolver.start()

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._srv.stop()
