import json
import urllib

from alice.uniproxy.library.settings import config, UNIPROXY_INSTALLATION

from alice.uniproxy.library.auth.tvm2 import get_messenger_service_token
from alice.uniproxy.library.auth.tvm2 import get_fanout_service_token
from alice.uniproxy.library.auth.blackbox import blackbox_client, BlackboxType, BlackboxError

from alice.uniproxy.library.global_counter import GlobalCounter, UnistatTiming
from alice.uniproxy.library.logging import Logger

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPResponse, HTTPError


# ====================================================================================================================
UNIPROXY_YAMB_HOST = config['yamb']['host']
UNIPROXY_YAMB_AUTH_PORT = config['yamb'].get('port', 443)
UNIPROXY_YAMB_AUTH_SECURE = config['yamb'].get('secure', True)
UNIPROXY_YAMB_AUTH_POOL_SIZE = config['yamb'].get('pool_size', 10)
UNIPROXY_YAMB_AUTH_NO_TVM = config['yamb'].get('disable_tvm', False)

UNIPROXY_FANOUT_HOST = config['messenger']['host']
UNIPROXY_FANOUT_AUTH_PORT = config['messenger'].get('auth_port', config['messenger']['port'])
UNIPROXY_FANOUT_AUTH_SECURE = config['messenger'].get('auth_secure', True)
UNIPROXY_FANOUT_AUTH_POOL_SIZE = config['messenger'].get('auth_pool_size', 10)
UNIPROXY_FANOUT_AUTH_NO_TVM = config['messenger'].get('disable_tvm', False)


def update_global_yamb_config():
    global UNIPROXY_YAMB_HOST
    global UNIPROXY_YAMB_AUTH_PORT
    global UNIPROXY_YAMB_AUTH_SECURE
    global UNIPROXY_YAMB_AUTH_POOL_SIZE
    global UNIPROXY_YAMB_AUTH_NO_TVM

    UNIPROXY_YAMB_HOST = config['yamb']['host']
    UNIPROXY_YAMB_AUTH_PORT = config['yamb'].get('port', 443)
    UNIPROXY_YAMB_AUTH_SECURE = config['yamb'].get('secure', True)
    UNIPROXY_YAMB_AUTH_POOL_SIZE = config['yamb'].get('pool_size', 10)
    UNIPROXY_YAMB_AUTH_NO_TVM = config['yamb'].get('disable_tvm', False)


def update_global_fanout_config():
    global UNIPROXY_FANOUT_HOST
    global UNIPROXY_FANOUT_AUTH_PORT
    global UNIPROXY_FANOUT_AUTH_SECURE
    global UNIPROXY_FANOUT_AUTH_POOL_SIZE
    global UNIPROXY_FANOUT_AUTH_NO_TVM

    UNIPROXY_FANOUT_HOST = config['messenger']['host']
    UNIPROXY_FANOUT_AUTH_PORT = config['messenger'].get('auth_port', config['messenger']['port'])
    UNIPROXY_FANOUT_AUTH_SECURE = config['messenger'].get('auth_secure', True)
    UNIPROXY_FANOUT_AUTH_POOL_SIZE = config['messenger'].get('auth_pool_size', 10)
    UNIPROXY_FANOUT_AUTH_NO_TVM = config['messenger'].get('disable_tvm', False)


# ====================================================================================================================
class MessengerAuthError(Exception):
    def __init__(self, initial_ex, scope=None):
        if scope is not None:
            scope = str(scope)

        if isinstance(initial_ex, HTTPError):
            if initial_ex.body:
                reason = initial_ex.body.decode("utf8")
            else:
                reason = 'unknown'

            try:
                reason = json.loads(reason)
            except Exception:
                reason = {
                    'data': {
                        'code': 'http_{}'.format(initial_ex.code),
                        'text': reason,
                    }
                }

            self.message = {
                "scope": scope if scope else "yamb",
                "code": str(reason["data"]["code"]),
                "text": str(reason["data"]["text"]),
            }
        elif isinstance(initial_ex, BlackboxError):
            self.message = {
                "scope": scope if scope else "blackbox",
                # This is not an error, see https://st.yandex-team.ru/MSSNGR-5435#5bfe57ccb9894a001c5dafc3
                "code": str(initial_ex.message),
                "text": str(initial_ex.code)
            }
        else:
            self.message = {
                "scope": scope if scope else "unknown",
                "code": str(initial_ex),
                "text": "-"
            }
        super().__init__()


# ====================================================================================================================
class AuthMode:
    Unknown = 0
    OAuth = 1
    OAuthTeam = 2
    YambAuth = 3
    Cookie = 4
    YambCookie = 5


# ====================================================================================================================
class AuthBase(object):
    def __init__(self, host, timeout=None, metrics=None, blackbox_getter=None):
        super().__init__()
        self._host = host
        self._request_timeout = timeout if timeout else 1.0
        self._logger = Logger.get('mssngr.auth')
        self._metrics = metrics
        self._blackbox = blackbox_getter if blackbox_getter else blackbox_client
        self._auth_mode = AuthMode.Unknown

    # ----------------------------------------------------------------------------------------------------------------
    def get_client(self) -> QueuedHTTPClient:
        """should be implemented in child class"""
        pass

    async def get_service_ticket(self):
        """should be implemented in child class"""
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def make_request_body(self, method):
        """should be implemented in child class"""
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def extract_guid_from_response(self, response):
        """should be implemented in child class"""
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def on_http_status(self, status):
        status_group = status // 100
        if status_group == 2:
            GlobalCounter.g_counters[self._metrics.HTTP200].increment()
        elif status_group == 4:
            if status == 401:
                GlobalCounter.g_counters[self._metrics.HTTP401].increment()
            elif status == 403:
                GlobalCounter.g_counters[self._metrics.HTTP403].increment()
            else:
                GlobalCounter.g_counters[self._metrics.HTTP4xx].increment()
        elif status_group == 5:
            if HTTPError.is_internal(status):
                # it's a metric for all timeouts and IO-errors
                GlobalCounter.g_counters[self._metrics.HTTP599].increment()
            else:
                GlobalCounter.g_counters[self._metrics.HTTP5xx].increment()

    # ----------------------------------------------------------------------------------------------------------------
    def on_bb_http_status(self, status):
        status_group = status // 100
        if status_group == 4:
            GlobalCounter.g_counters[self._metrics.Blackbox_4xx].increment()
        elif status_group == 5:
            if status == 599:
                GlobalCounter.g_counters[self._metrics.Blackbox_599].increment()
            else:
                GlobalCounter.g_counters[self._metrics.Blackbox_5xx].increment()

    # ----------------------------------------------------------------------------------------------------------------
    def on_success(self, http_status):
        if http_status:
            self.on_http_status(http_status)

        GlobalCounter.g_counters[self._metrics.AuthStatus[self._auth_mode].Ok].increment()

    # --------------------------------------------------------------------------------------------------------------------
    def on_fail(self, http_status, uniproxy=False, other=False, blackbox_status=0):
        if http_status:
            self.on_http_status(http_status)

        if blackbox_status:
            self.on_bb_http_status(blackbox_status)

        GlobalCounter.g_counters[self._metrics.AuthStatus[self._auth_mode].Fail].increment()

        if uniproxy:
            GlobalCounter.g_counters[self._metrics.InternalError].increment()

        if other:
            GlobalCounter.g_counters[self._metrics.Other].increment()

    # ----------------------------------------------------------------------------------------------------------------
    async def auth_user(self, token, token_type, y_cookie, y_yamb_cookie, icookie, ip, port, session_id, origin):
        exception = None

        try:
            if token:
                self._logger.debug('{}: authentication by token'.format(session_id))
                request = await self.prepare_auth_by_token_and_type(ip, port, token, token_type)
            elif y_cookie:
                request = await self.prepare_auth_by_session_id(ip, port, origin, y_cookie, y_yamb_cookie, icookie)
            elif y_yamb_cookie or icookie:
                request = await self.prepare_auth_by_yamb_session_id(user_cookie=y_yamb_cookie, icookie=icookie)
            else:
                self._logger.error('{}: no authentication data'.format(session_id))
                request = None

            if request is None:
                raise MessengerAuthError('no_acceptable_auth_data', scope='uniproxy')

            self._logger.debug('{}: fetching request {}'.format(session_id, request))
            response = await self.fetch_request(session_id, ip, request)

            self._logger.debug('{}: parsing response'.format(session_id))
            guid = self.extract_guid_from_response(response)

            self.on_success(response.code)
        except HTTPError as ex:
            self.on_fail(ex.code)
            self._logger.error(ex)
            exception = ex
        except BlackboxError as ex:
            self.on_fail(0, blackbox_status=ex.http_status)
            self._logger.error(ex)
            exception = ex
        except MessengerAuthError as ex:
            self.on_fail(0, other=True)
            self._logger.error(ex)
            raise
        except Exception as ex:
            self.on_fail(0, uniproxy=True)
            self._logger.exception(ex)
            exception = ex

        if exception:
            raise MessengerAuthError(exception)

        self._logger.info('{}: GUID {}'.format(session_id, guid))

        return guid

    # ----------------------------------------------------------------------------------------------------------------
    async def prepare_auth_by_token_and_type(self, client_ip, client_port, token, token_type):
        token_type_lower = token_type.lower() if token_type is not None else ''

        if token_type_lower == 'oauth' or token_type_lower == 'oauthteam':
            is_team = True if token_type_lower == 'oauthteam' else False
            return await self.prepare_auth_by_oauth_token(client_ip, client_port, token, is_team)
        if token_type_lower == 'yambauth':
            return await self.prepare_auth_by_yambauth_token(token)

    # ----------------------------------------------------------------------------------------------------------------
    async def prepare_auth_by_oauth_token(self, client_ip, client_port, token, is_team):
        self._auth_mode = AuthMode.OAuthTeam if is_team else AuthMode.OAuth
        blackbox_type = BlackboxType.Team if is_team else BlackboxType.Public

        with UnistatTiming(self._metrics.TimeBlackbox):
            user_ticket = await self._blackbox().ticket4oauth(token, client_ip, client_port, blackbox_type)

        request = self.prepare_request('oauth', {
            'X-YA-USER-TICKET': user_ticket,
            'X-YAMB-IS-TEAM': 'True' if is_team else 'False',
        })

        return request

    # ----------------------------------------------------------------------------------------------------------------
    async def prepare_auth_by_yambauth_token(self, token):
        self._auth_mode = AuthMode.YambAuth
        request = self.prepare_request('oauth', {
            'x-yamb-token': token,
            'x-yamb-token-type': 'YambAuth',
        })
        return request

    # ----------------------------------------------------------------------------------------------------------------
    async def prepare_auth_by_session_id(self, client_ip, client_port, origin, user_cookie, yamb_cookie, icookie):
        self._auth_mode = AuthMode.Cookie
        self._logger.debug('authentication by Session_id')

        team_domain = 'yandex-team' in origin

        bb_exception = None
        try:
            with UnistatTiming(self._metrics.TimeBlackbox):
                user_ticket = await self._blackbox().ticket4sessionid(user_cookie, client_ip, origin, client_port)
        except BlackboxError as ex:
            statuses = ['INVALID', 'EXPIRED', 'NOAUTH', 'DISABLED']
            if team_domain:
                bb_exception = ex
            elif (ex.http_status < 499 or ex.status_value in statuses) and (icookie or yamb_cookie):
                self._logger.info('fallback to yamb cookie auth')
                self._auth_mode = AuthMode.YambCookie
            else:
                bb_exception = ex

        if bb_exception:
            raise bb_exception

        if self._auth_mode == AuthMode.Cookie:
            request = self.prepare_request('oauth', {
                'X-YA-USER-TICKET': user_ticket,
                'X-YAMB-IS-TEAM': 'True' if team_domain else 'False',
            })
        elif self._auth_mode == AuthMode.YambCookie:
            request = await self.prepare_auth_by_yamb_session_id(user_cookie=yamb_cookie, icookie=icookie)

        return request

    # ----------------------------------------------------------------------------------------------------------------
    async def prepare_auth_by_yamb_session_id(self, user_cookie, icookie):
        self._auth_mode = AuthMode.YambCookie
        self._logger.debug('authentication by yamb_session_id')

        headers = {}

        if user_cookie:
            headers['X-YAMB-AUTH-COOKIE'] = user_cookie
        if icookie:
            headers['X-Ya-I'] = icookie

        request = self.prepare_request('session_id', headers)

        return request

    # ----------------------------------------------------------------------------------------------------------------
    def prepare_request(self, method, headers):
        request = HTTPRequest(
            '/meta_api/',
            method='POST',
            headers=headers,
            body=self.make_request_body(method=method),
            retries=3,
            request_timeout=self._request_timeout
        )
        return request

    # ----------------------------------------------------------------------------------------------------------------
    async def fetch_request(self, session_id, client_ip, request):
        service_ticket = await self.get_service_ticket()

        self._logger.debug('{}: headers {}'.format(session_id, request.headers))

        request.headers.update({
            'Host': self._host,
            'Content-Type': 'application/x-www-form-urlencoded',
            'X-Ya-Service-Ticket': service_ticket,
            'X-REQUEST-ID': session_id,
            'X-USER_IP': client_ip,
            'X-Rproxy-Client': UNIPROXY_INSTALLATION,
        })

        with UnistatTiming(self._metrics.Time):
            response = await self.get_client().fetch(request)

        return response


class UndefinedAuthStatusMetrics:
    Ok = 'mssngr_auth_undefined_ok_summ'
    Fail = 'mssngr_auth_undefined_fail_summ'


class OAuthStatusMetrics:
    Ok = 'mssngr_oauth_ok_summ'
    Fail = 'mssngr_oauth_fail_summ'


class OAuthTeamStatusMetrics:
    Ok = 'mssngr_oauthteam_ok_summ'
    Fail = 'mssngr_oauthteam_fail_summ'


class YambAuthStatusMetrics:
    Ok = 'mssngr_yambauth_ok_summ'
    Fail = 'mssngr_yambauth_fail_summ'


class CookieAuthStatusMetrics:
    Ok = 'mssngr_auth_cookie_ok_summ'
    Fail = 'mssngr_auth_cookie_fail_summ'


class YambCookieAuthStatusMetrics:
    Ok = 'mssngr_auth_yambcookie_ok_summ'
    Fail = 'mssngr_auth_yambcookie_fail_summ'


# ====================================================================================================================
class FanoutAuthMetrics:
    HTTP200 = 'mssngr_auth_fanout_200_summ'
    HTTP401 = 'mssngr_auth_fanout_401_summ'
    HTTP403 = 'mssngr_auth_fanout_403_summ'
    HTTP4xx = 'mssngr_auth_fanout_4xx_summ'
    HTTP5xx = 'mssngr_auth_fanout_5xx_summ'
    HTTP599 = 'mssngr_auth_fanout_599_summ'
    Blackbox_4xx = 'mssngr_auth_fanout_bb_4xx_summ'
    Blackbox_5xx = 'mssngr_auth_fanout_bb_5xx_summ'
    Blackbox_599 = 'mssngr_auth_fanout_bb_5xx_summ'
    Other = 'mssngr_auth_fanout_fail_summ'
    InternalError = 'mssngr_auth_fanout_internal_fail_summ'
    Time = 'fanout_auth_wait'
    TimeBlackbox = 'mssngr_auth_fanout_blackbox_time'

    AuthStatus = [
        UndefinedAuthStatusMetrics,
        OAuthStatusMetrics,
        OAuthTeamStatusMetrics,
        YambAuthStatusMetrics,
        CookieAuthStatusMetrics,
        YambCookieAuthStatusMetrics,
    ]


# ====================================================================================================================
class YambAuthMetrics:
    HTTP200 = 'mssngr_auth_yamb_200_summ'
    HTTP401 = 'mssngr_auth_yamb_401_summ'
    HTTP403 = 'mssngr_auth_yamb_403_summ'
    HTTP4xx = 'mssngr_auth_yamb_4xx_summ'
    HTTP5xx = 'mssngr_auth_yamb_5xx_summ'
    HTTP599 = 'mssngr_auth_yamb_599_summ'
    Blackbox_4xx = 'mssngr_auth_yamb_bb_4xx_summ'
    Blackbox_5xx = 'mssngr_auth_yamb_bb_5xx_summ'
    Blackbox_599 = 'mssngr_auth_yamb_bb_5xx_summ'
    Other = 'mssngr_auth_yamb_fail_summ'
    InternalError = 'mssngr_auth_yamb_internal_fail_summ'
    Time = 'mssngr_auth_wait'
    TimeBlackbox = 'mssngr_auth_yamb_blackbox_time'

    AuthStatus = [
        UndefinedAuthStatusMetrics,
        OAuthStatusMetrics,
        OAuthTeamStatusMetrics,
        YambAuthStatusMetrics,
        CookieAuthStatusMetrics,
        YambCookieAuthStatusMetrics,
    ]


# ====================================================================================================================
class FanoutAuth(AuthBase):
    def __init__(self, timeout=None, blackbox=None):
        super().__init__(UNIPROXY_FANOUT_HOST, timeout, metrics=FanoutAuthMetrics, blackbox_getter=blackbox)

    # ----------------------------------------------------------------------------------------------------------------
    def get_client(self):
        self._logger.debug('fanout auth get_client')
        client = QueuedHTTPClient.get_client(
            host=UNIPROXY_FANOUT_HOST,
            port=UNIPROXY_FANOUT_AUTH_PORT,
            secure=UNIPROXY_FANOUT_AUTH_SECURE,
            pool_size=UNIPROXY_FANOUT_AUTH_POOL_SIZE,
            queue_size=300,
            retry_on_timeout=False
        )
        self._logger.debug('fanout auth get_client {}'.format(client))
        return client

    # ----------------------------------------------------------------------------------------------------------------
    def make_request_body(self, method):
        return json.dumps({'method': 'oauth'}).encode('utf-8')

    # ----------------------------------------------------------------------------------------------------------------
    def extract_guid_from_response(self, response: HTTPResponse):
        data = response.json()
        guid = data.get('guid')
        if guid and guid.lower() == 'none':
            guid = None
        return guid

    # ----------------------------------------------------------------------------------------------------------------
    async def get_service_ticket(self):
        if not UNIPROXY_FANOUT_AUTH_NO_TVM:
            service_ticket = await get_fanout_service_token()
        else:
            service_ticket = ''
        return service_ticket


# ====================================================================================================================
class YambAuth(AuthBase):
    def __init__(self, timeout=None, blackbox=None):
        super().__init__(UNIPROXY_YAMB_HOST, timeout, metrics=YambAuthMetrics, blackbox_getter=blackbox)

    # ----------------------------------------------------------------------------------------------------------------
    def get_client(self):
        self._logger.debug('yamb auth get_client')
        client = QueuedHTTPClient.get_client(
            host=UNIPROXY_YAMB_HOST,
            port=UNIPROXY_YAMB_AUTH_PORT,
            secure=UNIPROXY_YAMB_AUTH_SECURE,
            pool_size=UNIPROXY_YAMB_AUTH_POOL_SIZE,
            queue_size=300
        )
        self._logger.debug('yamb auth get_client {}'.format(client))
        return client

    # ----------------------------------------------------------------------------------------------------------------
    def make_request_body(self, method):
        return urllib.parse.urlencode({
            'request': json.dumps({
                'method': method,
                'params': {},
            })
        }).encode('utf-8')

    # ----------------------------------------------------------------------------------------------------------------
    def extract_guid_from_response(self, response: HTTPResponse):
        self._logger.debug(response)
        self._logger.debug(response.body)
        data = response.json()
        guid = data.get('data', {}).get('guid')
        if guid and guid.lower() == 'none':
            guid = None
        return guid

    # ----------------------------------------------------------------------------------------------------------------
    async def get_service_ticket(self):
        if not UNIPROXY_YAMB_AUTH_NO_TVM:
            service_ticket = await get_messenger_service_token()
        else:
            service_ticket = ''
        return service_ticket
