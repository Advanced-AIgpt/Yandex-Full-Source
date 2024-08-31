import time
import re
import random
import urllib
import tornado.httputil
import tornado.gen
import tornado.web

from tornado.ioloop import IOLoop

from raven.contrib.tornado import SentryMixin

from alice.uniproxy.library.common_handlers import CommonWebSocketHandler
from alice.uniproxy.library.unisystem.unisystem import UniSystem
from alice.uniproxy.library.unisystem.unisystem import UNIWS_UUID_HEADER
from alice.uniproxy.library.unisystem.unisystem import UNIWS_AUTHTOKEN_HEADER

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import Srcrwr, GraphOverrides

from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter import GlobalCounter


USE_LOCAL_DOMAIN_RESTRICTION_PATTERN = config.get('local_domain_restriction', False)


DOMAIN_RESTRICTION_PATTERN_LOCAL = re.compile(
    r'((\.yandex\.(ru|by|ua|kz|com|com\.tr|com\.am|com\.ge|az|co\.il|kg|lv|lt|md|tj|tm|uz|ee|fr)$)|'
    + r'(^(www\.)?ya\.ru$)|'
    + r'(^(www\.)?yandex\.(ru|by|ua|kz|com|com\.tr|com\.am|com\.ge|az|co\.il|kg|lv|lt|md|tj|tm|uz|ee|fr)$)|'
    + r'(\.yandex-team\.ru$)|(local\.yandex\.ru:3443))'
)

DOMAIN_RESTRICTION_PATTERN = re.compile(
    r'((\.yandex\.(ru|by|ua|kz|com|com\.tr|com\.am|com\.ge|az|co\.il|kg|lv|lt|md|tj|tm|uz|ee|fr)$)|'
    + r'(^(www\.)?ya\.ru$)|'
    + r'(^(www\.)?yandex\.(ru|by|ua|kz|com|com\.tr|com\.am|com\.ge|az|co\.il|kg|lv|lt|md|tj|tm|uz|ee|fr)$)|'
    + r'(\.yandex-team\.ru$))'
)


class UniWebSocket(SentryMixin, CommonWebSocketHandler):
    unistat_handler_name = 'uni_ws'

    def __init__(self, *args, **kwargs):
        self._bad_origin = False
        self._logger = Logger.get('.uniwebsocket')
        super().__init__(*args, **kwargs)
        self.system = None
        self._url = 'uni.ws'
        self.status = 0
        self.last_processors_count = 0

    def get_compression_options(self):
        return {}

    @tornado.web.asynchronous
    def get(self, *args, **kwargs):
        if GlobalState.is_offline():
            self.set_status(503)
            self.finish('try again later')
            return
        super(UniWebSocket, self).get(*args, **kwargs)

    def open(self):
        self.set_nodelay(True)
        self._logger.debug('Start new connection')

        self.client_ip = self.request.headers.get(
            'X-Real-Ip',
            self.request.headers.get('X-Forwarded-For', self.request.remote_ip))

        self.client_port = self.request.headers.get('X-Real-Port')

        test_ids = self.get_query_arguments('test-id')
        self.system = UniSystem(self, ipaddr=self.client_ip, client_port=self.client_port, test_ids=test_ids)

        self.system.origin = self.request.headers.get('X-Forwarded-Host', self.request.headers.get('Host'))

        self.system.x_yamb_token = self.request.headers.get(
            'x-yamb-token',
            self.request.headers.get('X-Yamb-Token'))

        self.system.x_yamb_token_type = self.request.headers.get(
            'x-yamb-token-type',
            self.request.headers.get('X-Yamb-Token-Type'))

        self.system.user_agent = self.request.headers.get('User-Agent')

        #
        #   SECAUDIT-2668, VOICESERV-1857
        #   disable cookie auth in case of invalid origin
        #
        if not self._bad_origin:
            self.system.x_yamb_cookie = self.request.headers.get(
                'Cookie',
                self.request.headers.get('X-Yamb-Cookie'))
        else:
            self.system.x_yamb_cookie = None

        if self.system.x_yamb_cookie:
            parsed_cookies = tornado.httputil.parse_cookie(self.system.x_yamb_cookie)
            self.system.y_session_id = parsed_cookies.get('Session_id')
            self.system.y_yamb_session_id = parsed_cookies.get('yamb_session_id')
            self.system.icookie = parsed_cookies.get('i')

        self.system.srcrwr = Srcrwr(self.request.headers, self.get_query_arguments(Srcrwr.CGI, strip=True))
        self.system.graph_overrides = GraphOverrides(self.get_query_arguments(GraphOverrides.CGI, strip=True))

        self.status = 200
        self.start_time = time.monotonic()
        self.total_sent = 0

        GlobalCounter.UNIPRX_WS_OPEN_SUMM.increment()
        GlobalCounter.UNIPRX_WS_CURRENT_AMMX.increment()

        # activity_check_period_seconds will be updated from ITS in first SetState.
        self.activity_check_period_seconds = random.randint(600, 1200)
        self.timer_activity = IOLoop.current().call_later(self.activity_check_period_seconds, self.check_processors)

    def on_message(self, message):
        GlobalCounter.UNIPRX_WS_MESSAGE_SUMM.increment()
        self.system.on_message(message)

    def check_processors(self):
        events_processed = self.system.total_processors_count - self.last_processors_count
        if events_processed > 0:
            self.timer_activity = IOLoop.current().call_later(self.activity_check_period_seconds, self.check_processors)
            GlobalCounter.UNIPRX_WS_LONG_ACTIVE_SUMM.increment()
            self.last_processors_count = self.system.total_processors_count
        else:
            self.close(reason='timeout+inactivity')
            GlobalCounter.UNIPRX_WS_NO_ACTIVE_SUMM.increment()

    def on_close(self):
        GlobalCounter.UNIPRX_WS_CLOSE_SUMM.increment()
        GlobalCounter.UNIPRX_WS_CURRENT_AMMX.decrement()
        self._logger.debug('WebSocket closed')
        Logger.access(
            '{}?key={}&uuid={}'.format(
                self._url,
                self.system.session_data.get('key', ''),
                self.system.session_data.get('uuid', ''),
            ),
            self.status,
            self.client_ip,
            1000 * (time.monotonic() - self.start_time)
        )
        if self.timer_activity:
            IOLoop.current().remove_timeout(self.timer_activity)
            self.timer_activity = None
        self.system.close()
        self.system = None

    def check_origin(self, origin):
        if USE_LOCAL_DOMAIN_RESTRICTION_PATTERN:
            res = DOMAIN_RESTRICTION_PATTERN_LOCAL.search(urllib.parse.urlparse(origin).netloc) is not None
        else:
            res = DOMAIN_RESTRICTION_PATTERN.search(urllib.parse.urlparse(origin).netloc) is not None

        if not res:
            self._logger.error('BAD ORIGIN HEADER:', origin)
            self._bad_origin = True
        return True

    def set_default_headers(self):
        origin = self.request.headers.get('Origin')
        if origin is None:
            origin = self.request.headers.get('Sec-Websocket-Origin', None)
        if origin and self.check_origin(origin):
            self.set_header('Access-Control-Allow-Origin', origin)
            self.set_header('Access-Control-Allow-Credentials', 'true')


class HUniWebSocket(UniWebSocket):
    unistat_handler_name = 'huni_ws'

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._url = 'huni.ws'

    def open(self):
        super().open()
        if (
            UNIWS_UUID_HEADER not in self.request.headers
            or UNIWS_AUTHTOKEN_HEADER not in self.request.headers
        ):
            self.status = 400
            self.close(code=400, reason='uuid or auth_token is not provided')
        else:
            self.system.process_headers()

    def check_origin(self, origin):
        if USE_LOCAL_DOMAIN_RESTRICTION_PATTERN:
            return DOMAIN_RESTRICTION_PATTERN_LOCAL.search(urllib.parse.urlparse(origin).netloc) is not None
        else:
            return DOMAIN_RESTRICTION_PATTERN.search(urllib.parse.urlparse(origin).netloc) is not None
