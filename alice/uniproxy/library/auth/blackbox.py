from urllib.parse import urlparse

from enum import Enum, unique
import requests

import tornado.gen

import ticket_parser2 as tp2
from ticket_parser2.low_level import UserContext

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPError
from alice.uniproxy.library.async_http_client import RTLogHTTPRequest
from alice.uniproxy.library.settings import config

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger
from rtlog import null_logger

from .tvm2 import tvm_client

# ====================================================================================================================
g_user_context = None


# ====================================================================================================================
def _join_cgi(**kwargs):
    if 'user_port' in kwargs and kwargs['user_port'] is None:
        del kwargs['user_port']
    return '?' + '&'.join('{}={}'.format(key, value) for key, value in kwargs.items())


def _make_blackbox_exception(response):
    jresp = response.json()
    return BlackboxError(
        jresp.get('error', ''),
        jresp.get('status', jresp.get('exception', {}).get('value', 'INVALID')),
        response.code,
        status_value=jresp.get('status', {}).get('value', '')
    )


# ====================================================================================================================
class BlackboxError(Exception):
    def __init__(self, message, code, http_status=0, status_value=None):
        super().__init__()
        self.message = message
        self.code = code
        self.http_status = http_status
        self.status_value = status_value

    def __repr__(self):
        return 'HTTP {}: {}'.format(self.code, self.message)

    def __str__(self):
        return self.__repr__()


@unique
class BlackboxType(Enum):
    Public = 'blackbox'
    Team = 'blackbox_yt'


# ====================================================================================================================
class BlackboxClient(object):
    def __init__(self, blackbox_type):
        self._config = config.get(blackbox_type.value, {})
        self._service_id = self._config.get('service_id')

        self._url = self._config.get('url')
        if not self._url:
            raise BlackboxError(
                'Blackbox is not configured for {}'.format(blackbox_type.value),
                'CONFIGURATION_ERROR'
            )

        url_parsed = urlparse(self._url)
        port = url_parsed.port
        if port is None:
            port = 443 if url_parsed.scheme == 'https' else 80

        self._host = url_parsed.hostname
        self._http_client = QueuedHTTPClient.get_client(
            host=self._host,
            port=port,
            pool_size=10,
            secure=bool(url_parsed.scheme == 'https')
        )

    async def make_request(self, path: str, rt_log=null_logger(), rt_log_label=None):
        b_url = '{}{}'.format(urlparse(self._url).path, path)
        service_ticket = await tvm_client().service_ticket_for(self._service_id)

        headers = {
            'Host': self._host,
            'X-Ya-Service-Ticket': service_ticket,
        }

        default_timeout = self._config.get('timeout', 0.2)

        if not rt_log_label:
            rt_log_label = 'untitled blackbox request'

        request = RTLogHTTPRequest(
            b_url,
            headers=headers,
            request_timeout=self._config.get('request_timeout', default_timeout),
            retries=self._config.get('retries', 5),
            rt_log=rt_log,
            rt_log_label='blackbox: ' + rt_log_label,
        )
        return await self._http_client.fetch(request)


# ====================================================================================================================
class BlackboxTicket(object):
    def __init__(self):
        self._log = Logger.get('.auth.blackbox')
        self._clients = {}

        for b_type in BlackboxType:
            self._clients[b_type.value] = BlackboxClient(b_type)

    async def ticket4oauth(
            self,
            token: str,
            client_ip: str,
            client_port: int = None,
            environment: BlackboxType = BlackboxType.Public,
            rt_log=null_logger(),
    ) -> str:
        url = _join_cgi(
            method='oauth',
            format='json',
            get_user_ticket='yes',
            oauth_token=token,
            userip=client_ip,
            user_port=client_port,
        )
        return await self._get_user_ticket(url, environment, rt_log=rt_log, rt_log_label='ticket4oauth')

    async def ticket4sessionid(
            self,
            sessionid: str,
            client_ip: str,
            host: str,
            client_port: int = None,
            rt_log=null_logger(),
    ) -> str:
        url = _join_cgi(
            method='sessionid',
            format='json',
            get_user_ticket='yes',
            sessionid=sessionid,
            userip=client_ip,
            host=host,
            user_port=client_port,
        )
        return await self._get_user_ticket(url, BlackboxType.Team if 'yandex-team' in host else BlackboxType.Public,
                                           rt_log=rt_log, rt_log_label='ticket4sessionid')

    async def _make_blackbox_request(self, path: str, blackbox_type: BlackboxType,
                                     rt_log=null_logger(), rt_log_label=None):
        try:
            response = await self._clients[blackbox_type.value].make_request(path, rt_log, rt_log_label)
            GlobalCounter.increment_error_code('blackbox', response.code)
            return response
        except HTTPError as ex:
            GlobalCounter.increment_error_code('blackbox', ex.code)
            raise BlackboxError('Bad Blackbox response', ex.code, ex.code)
        except (TimeoutError, tornado.gen.TimeoutError):
            GlobalCounter.increment_error_code('blackbox', 600)
            raise BlackboxError('Blackbox timeout', 'TIMEOUT', 599)
        except Exception as ex:
            GlobalCounter.increment_error_code('blackbox', 601)
            self._log.exception(ex)
            raise BlackboxError(
                'Blackbox error',
                'exception {}'.format(type(ex))
            )

    async def _get_from_bb_response(self, key: str, url: str, blackbox_type: BlackboxType,
                                    rt_log, rt_log_label: str):
        response = await self._make_blackbox_request(url, blackbox_type, rt_log, rt_log_label)
        jresp = response.json()
        value = jresp.get(key)
        if not value:
            raise _make_blackbox_exception(response)
        return value

    async def _get_user_ticket(self, url: str, blackbox_type: BlackboxType,
                               rt_log=null_logger(), rt_log_label='user_ticket') -> str:
        return await self._get_from_bb_response('user_ticket', url, blackbox_type, rt_log, rt_log_label)

    async def uid4oauth(
            self,
            token: str,
            client_ip: str,
            client_port: int = None,
            rt_log=null_logger()
    ) -> None:
        url = _join_cgi(
            method='oauth',
            format='json',
            oauth_token=token,
            userip=client_ip,
            user_port=client_port,
            aliases=13,
        )
        response = await self._make_blackbox_request(url, BlackboxType.Public, rt_log, 'uid4oauth')
        jresp = response.json()
        if jresp.get('status', {}).get('value') == 'VALID':
            uid = jresp.get('oauth', {}).get('uid')
            staff_login = jresp.get('aliases', {}).get('13')
            return (uid, staff_login)
        else:
            raise _make_blackbox_exception(response)

    async def login4sessionid(
            self,
            blackbox_type: BlackboxType,
            sessionid: str,
            client_ip: str,
            host: str,
            client_port: int = None,
            rt_log=null_logger(),
    ) -> str:
        url = _join_cgi(
            method='sessionid',
            format='json',
            sessionid=sessionid,
            userip=client_ip,
            user_port=client_port,
            host=host,
        )
        return await self._get_from_bb_response('login', url, blackbox_type, rt_log, 'login4sessionid')

    async def login4oauth(
            self,
            blackbox_type: BlackboxType,
            oauth_token: str,
            client_ip: str,
            client_port: int = None,
            rt_log=null_logger(),
    ) -> str:
        url = _join_cgi(
            method='oauth',
            format='json',
            oauth_token=oauth_token,
            userip=client_ip,
            user_port=client_port,
        )
        return await self._get_from_bb_response('login', url, blackbox_type, rt_log, 'login4oauth')


# ====================================================================================================================
_g_blackbox_client = None


# ====================================================================================================================
def blackbox_client() -> BlackboxTicket:
    global _g_blackbox_client
    if _g_blackbox_client is None:
        _g_blackbox_client = BlackboxTicket()
    return _g_blackbox_client


# ====================================================================================================================
def init_tvm():
    global g_user_context
    tvm_keys = requests.get(
        '{tvm_api_url}/2/keys?lib_version={version}'.format(
            tvm_api_url=config.get('tvm', {}).get('url', 'https://tvm-api.yandex.net'),
            version=tp2.__version__,
        )
    ).content
    g_user_context = UserContext(tp2.BlackboxEnv.Prod, tvm_keys.decode('utf-8'))
