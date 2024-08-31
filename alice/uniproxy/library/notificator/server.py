import tornado.ioloop
import tornado.httpserver
import tornado.web
import tornado.gen

from alice.uniproxy.library.common_handlers import PingHandler
from alice.uniproxy.library.common_handlers import UnistatHandler
from alice.uniproxy.library.common_handlers import StartHookHandler
from alice.uniproxy.library.common_handlers import StopHookHandler
from alice.uniproxy.library.common_handlers import InfoHandler

from alice.uniproxy.library.resolvers import RTCResolver

from alice.uniproxy.library.settings import QLOUD_HTTP_PORT
from alice.uniproxy.library.settings import UNIPROXY_MAX_FORKS
from alice.uniproxy.library.settings import config

import alice.uniproxy.library.notificator.delivery as delivery
from alice.uniproxy.library.notificator.notifications import DeletePersonalCardsHandler


RTC_DISCOVERY_ENABLED = config.get('delivery', {}).get('rtc_resolver', {}).get('enabled', False)


# ====================================================================================================================
class NotificatorServer(object):
    def __init__(self, *args, **kwargs):
        super(NotificatorServer, self).__init__()

        self._port = kwargs.get('port', QLOUD_HTTP_PORT)

        self._subway_port = kwargs.get('subway_port', config.get('subway', {}).get('port', 7777))

        self._procs = kwargs.get('procs', config.get('notificator', {}).get('procs', UNIPROXY_MAX_FORKS))

        delivery.MOCK_DELIVERY = kwargs.get("mock_delivery", False)
        delivery.NO_REMOVE_MISSING = kwargs.get("no_remove_missing", False)

        if RTC_DISCOVERY_ENABLED:
            self._resolver = RTCResolver.instance()

        self._app = tornado.web.Application([
            (r'/personal_cards/delete', DeletePersonalCardsHandler),
            (r'/delivery/sup', delivery.DeliverySupHandler),
            (r'/delivery/sup_card', delivery.DeliverySupCardHandler),

            # System
            (r'/ping', PingHandler),
            (r'/info', InfoHandler),
            (r'/unistat', UnistatHandler),
            (r"/stop_hook", StopHookHandler),
            (r"/start_hook", StartHookHandler),
        ])

    # ----------------------------------------------------------------------------------------------------------------
    def start(self):
        self._srv = tornado.httpserver.HTTPServer(self._app)
        self._srv.bind(self._port)
        self._srv.start(self._procs)

        if RTC_DISCOVERY_ENABLED:
            self._resolver.start()

    # ----------------------------------------------------------------------------------------------------------------
    def stop(self):
        self._srv.stop()
