import tornado.gen
import os
import json
import time
import paramiko

from base64 import urlsafe_b64encode
from urllib.parse import urlencode
from urllib.parse import urlparse
from rtlog import null_logger

import ticket_parser2 as tp2
from ticket_parser2.low_level import ServiceContext

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.async_http_client import QueuedHTTPClient
from alice.uniproxy.library.async_http_client import RTLogHTTPRequest
from alice.uniproxy.library.logging.log import Logger

from .tvm_daemon_client import TvmDaemonClient


TVM_API_URL = config.get('tvm', {}).get('url', 'https://tvm-api.yandex.net')
TVM_TOOL_PORT = config.get('tvm', {}).get('tvm_tool_port', 1)

TVM_API_HOST = None
TVM_API_PORT = None
TVM_API_SECURE = None
if TVM_API_URL:
    TVM_API_TUPLE = urlparse(TVM_API_URL)
    TVM_API_HOST = TVM_API_TUPLE.hostname
    if TVM_API_TUPLE.scheme == 'https':
        TVM_API_SECURE = True
        TVM_API_PORT = 443
    else:
        TVM_API_SECURE = False
        TVM_API_PORT = 80
    if TVM_API_TUPLE.port:
        TVM_API_PORT = TVM_API_TUPLE.port

TVM_APP_SECRET = config.get('tvm_app_secret')
TVM_TICKET_KEEPALIVE_PERIOD = config.get('tvm', {}).get('ticket_keepalive_period', 60 * 60)
TICKETS_CACHE = {}


# ====================================================================================================================
class TVMError(Exception):
    def __init__(self, message):
        super(TVMError, self).__init__('TVMError({})'.format(message))


# ====================================================================================================================
class TVMToolClient(object):
    """
        Please use TvmDaemonClient or TvmDaemon.
        This legacy class makes direrect requests to global tvm server (see second part of service_ticket_for).
    """

    def __init__(self, tvm_token, service_id, port=TVM_TOOL_PORT):
        super(TVMToolClient, self).__init__()
        self._log = Logger.get('qtvmcli')
        self._token = tvm_token
        self._service_id = service_id
        self._ext_client = None
        self._daemon_client = TvmDaemonClient(tvm_token, port, service_id)

    @tornado.gen.coroutine
    def tickets(self, dest_id):
        service_name = ''
        service_ticket = ''

        try:
            service_ticket = yield self._daemon_client.get_service_ticket_for(dest_id)
        except Exception as ex:
            service_name = None
            service_ticket = None
            self._log.exception(ex)

        raise tornado.gen.Return((service_name, service_ticket))

    @tornado.gen.coroutine
    def check_service_ticket(self, ticket):
        ok, info = yield self._daemon_client.check_service_ticket(ticket)
        if ok:
            self._log.debug('server ticket is Ok')
        else:
            raise TVMError(info)

    def make_signs(self, data):
        res = []
        for key in paramiko.Agent().get_keys():
            res.append(urlsafe_b64encode(key.sign_ssh_data(data)))
        return res

    @tornado.gen.coroutine
    def service_ticket_for(self, dst, rt_log=null_logger(), rt_log_label=''):
        qloud_tvm_token = config.get('qloud_tvm_token')
        if qloud_tvm_token:
            cached = TICKETS_CACHE.get(dst)
            ts = int(time.time())
            if cached and ts < (cached['ts'] + TVM_TICKET_KEEPALIVE_PERIOD):
                return cached['ticket']

            _, ticket = yield self.tickets(dst)
            if not ticket:
                raise Exception('qloud_tvm: fail getting service_ticket for {}'.format(dst))

            TICKETS_CACHE[dst] = {
                'ticket': ticket,
                'ts': ts,
            }
            return ticket
        # else: outside qloud request tvm directly

        dst = str(dst)
        ticket = TICKETS_CACHE.get(dst)
        ts = int(time.time())

        if ticket and ticket['ts'] + TVM_TICKET_KEEPALIVE_PERIOD > ts:
            return ticket['ticket']

        self._log.debug('service ticket for {} not found in cache, request tvm={}'.format(dst, TVM_API_URL))
        ts = int(time.time())
        src = config['client_id']

        request = RTLogHTTPRequest(
            '/2/keys?lib_version={version}'.format(version=tp2.__version__),
            headers={
                'Host': TVM_API_HOST,
                'Connection': 'Keep-Alive',
            },
            request_timeout=0.1,
            retries=3,
            rt_log=rt_log,
            rt_log_label='{0}/tvm_api_keys'.format(rt_log_label)
        )

        if self._ext_client is None:
            self._ext_client = QueuedHTTPClient.get_client(
                host=TVM_API_HOST,
                port=TVM_API_PORT,
                secure=TVM_API_SECURE,
                pool_size=1,
            )

        response = yield self._ext_client.fetch(request)
        tvm_keys = response.body.decode('utf-8')

        params = {
            'grant_type': 'client_credentials',
            'src': src,
            'dst': dst,
            'ts': ts,
        }
        if TVM_APP_SECRET is None:
            params['grant_type'] = 'sshkey'
            params['login'] = os.getenv('USER')
            ssh_signs = self.make_signs('{}|{}|{}'.format(ts, src, dst))
            if not ssh_signs:
                raise Exception('has not qloud tvm token and can not get signature using ssh-auth')
            params['ssh_sign'] = ssh_signs[0]
        else:
            service_context = ServiceContext(src, TVM_APP_SECRET, tvm_keys)
            params['sign'] = service_context.sign(ts, dst)

        # Getting tickets
        default_timeout = config['blackbox'].get('timeout', 0.2)
        request = RTLogHTTPRequest(
            '/2/ticket/',
            headers={
                'Host': TVM_API_HOST,
                'Connection': 'Keep-Alive',
                'Content-Type: ': 'application/x-www-form-urlencoded',
            },
            method='POST',
            body=urlencode(params).encode('utf-8'),
            request_timeout=config['blackbox'].get('request_timeout', default_timeout),
            rt_log=rt_log,
            rt_log_label='{0}/tvm_api_ticket'.format(rt_log_label)
        )

        response = yield self._ext_client.fetch(request)
        ticket_response = json.loads(response.body.decode('utf-8'))
        service_ticket = ticket_response.get(str(dst), {}).get('ticket')
        if not service_ticket:
            raise Exception('fail getting service_ticket for {}'.format(dst))

        TICKETS_CACHE[dst] = {
            'ts': ts,
            'ticket': service_ticket,
        }
        return service_ticket


# ====================================================================================================================
_g_tvm_tool_client = None


# ====================================================================================================================
def tvm_client() -> TVMToolClient:
    global _g_tvm_tool_client
    if _g_tvm_tool_client is None:
        _g_tvm_tool_client = TVMToolClient(
            config['qloud_tvm_token'],
            config['client_id']
        )
    return _g_tvm_tool_client


def make_tvm_daemon_client(self_id) -> TvmDaemonClient:
    return TvmDaemonClient(
        config['qloud_tvm_token'],
        TVM_TOOL_PORT,
        self_id,
        http_client_kwargs=dict(pool_size=1),
    )


# ====================================================================================================================
@tornado.gen.coroutine
def get_messenger_service_token():
    ticket = yield tvm_client().service_ticket_for(config.get('yamb', {}).get('service_id'))
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def check_service_ticket(ticket) -> None:
    yield tvm_client().check_service_ticket(ticket)


# ====================================================================================================================
@tornado.gen.coroutine
def get_fanout_service_token():
    ticket = yield tvm_client().service_ticket_for(config.get('messenger', {}).get('service_id_fanout'))
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_music(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['music']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_music')
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_contacts(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['contacts']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_contacts')
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_smart_home(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['smart_home']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_smart_home')
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_data_sync(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['data_sync']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_data_sync')
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_memento(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['memento']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_memento')
    return ticket


# ====================================================================================================================
@tornado.gen.coroutine
def service_ticket_for_notificator(rt_log=null_logger()) -> str:
    ticket = yield tvm_client().service_ticket_for(config['notificator']['service_id'],
                                                   rt_log=rt_log, rt_log_label='service_ticket_for_notificator')
    return ticket


# ====================================================================================================================

async def authenticate_tvm_app(tvm_ticket, logger, client=None, self_id=None):
    client = client or make_tvm_daemon_client(self_id)
    ok, info = await client.check_service_ticket(tvm_ticket)
    if ok and info:
        src_tvm_id = info
        logger.info('Tvm app with id=%s authenticated', src_tvm_id)
        return src_tvm_id
    elif not ok and info:
        logger.warning('/tvm/checksrv (%s) returned error: %s', tvm_ticket, info)
    else:
        logger.error('Invalid response from /tvm/checksrv')
    return None


class PersonalTvmClient:
    def __init__(self, target_service_tvm_id, *args, **kwargs):
        self.client = TvmDaemonClient(*args, **kwargs)
        self.target_service_tvm_id = target_service_tvm_id
        self.self_alias = self.client.self_alias

    async def get_service_ticket(self):
        return await self.client.get_service_ticket_for(self.target_service_tvm_id)
