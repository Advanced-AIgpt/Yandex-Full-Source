import base64
import time
import tornado.gen
import tornado.escape

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPError
from alice.uniproxy.library.async_http_client import RTLogHTTPRequest
from alice.uniproxy.library.auth.tvm2 import service_ticket_for_memento
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import MEMENTO_GET_REQUEST_HGRAM, MEMENTO_UPDATE_REQUEST_HGRAM
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import conducting_experiment

from alice.memento.proto.api_pb2 import TReqGetAllObjects, TRespGetAllObjects, TReqChangeUserObjects

from rtlog import null_logger

settings = config['memento']
CONNECT_TIMEOUT = settings.get('connect_timeout', 0.1)


# for mocking
def _get_client(url):
    return QueuedHTTPClient.get_client_by_url(
        url,
        pool_size=settings.get('pool_size', 1),
        connect_timeout=CONNECT_TIMEOUT,
    )


class Memento:
    @staticmethod
    def parse_response(content):
        resp_get = TRespGetAllObjects()
        if content:
            resp_get.ParseFromString(content)
        # base64 of raw response. It needs because we don't want wait changes in protobufs api
        return base64.b64encode(content).decode('utf-8')

    def __init__(self, user_ticket_future, rt_log=null_logger(), host=None, responses_storage=None, request_info={}):
        self.logger = Logger.get('.backends.memento')
        self.rt_log = rt_log
        self.host = host or settings['host']
        self.service_ticket = None
        self.headers = {}
        self.user_ticket = None
        self.user_ticket_future = user_ticket_future
        self.responses_storage = responses_storage
        self.store_responses = conducting_experiment('context_load_diff', request_info)
        if self.store_responses:
            self.req_id = request_info.get('header', {}).get('request_id')

    @tornado.gen.coroutine
    def _get_service_ticket(self):
        try:
            self.service_ticket = yield service_ticket_for_memento(rt_log=self.rt_log)
        except Exception as exc:
            self.logger.exception('memento: fail getting service ticket: ' + str(exc))

    @tornado.gen.coroutine
    def _get_user_ticket(self):
        try:
            self.user_ticket = yield self.user_ticket_future()
        except Exception as exc:
            self.logger.exception('memento: fail getting user ticket: ' + str(exc))

    @tornado.gen.coroutine
    def prepare(self):
        if not self.service_ticket:
            yield self._get_service_ticket()

        if not self.user_ticket:
            yield self._get_user_ticket()

    @tornado.gen.coroutine
    def get(self, surface_id):
        yield self.prepare()
        if not self.user_ticket:
            return None

        headers = {
            'X-Ya-Service-Ticket': self.service_ticket,
            'X-Ya-User-Ticket': self.user_ticket,
            'Content-Type': 'application/protobuf',
        }

        host = self.host + settings['get_url']

        memento_req = TReqGetAllObjects()
        memento_req.SurfaceId.append(surface_id)

        request = RTLogHTTPRequest(
            host,
            method='POST',
            body=memento_req.SerializeToString(),
            headers=headers,
            request_timeout=settings['request_timeout'],
            retries=settings['retries'],
            rt_log=self.rt_log,
            rt_log_label='memento_get',
        )

        begin = time.monotonic()
        try:
            if self.store_responses and self.req_id:
                response = yield _get_client(host).fetch(request, raise_error=False)

                self.responses_storage.store(self.req_id, 'memento', response)

                if response.code // 100 != 2:
                    raise HTTPError(response.code, response.body, response.request)
            else:
                response = yield _get_client(host).fetch(request)

            ret = self.parse_response(response.body)
            GlobalCounter.MEMENTO_GET_REQUEST_OK_SUMM.increment()
            GlobalTimings.store(MEMENTO_GET_REQUEST_HGRAM, response.response_time())
            return ret
        except Exception as exc:
            GlobalTimings.store(MEMENTO_GET_REQUEST_HGRAM, time.monotonic() - begin)
            if isinstance(exc, HTTPError):
                if exc.code == 403:
                    GlobalCounter.MEMENTO_GET_REQUEST_FORBIDDEN_SUMM.increment()
                    return None
            else:
                self.logger.error('memento error: {}'.format(exc))
            GlobalCounter.MEMENTO_GET_REQUEST_FAIL_SUMM.increment()
            raise

    @tornado.gen.coroutine
    def update(self, data):
        yield self.prepare()
        if not self.user_ticket:
            return

        headers = {
            'X-Ya-Service-Ticket': self.service_ticket,
            'X-Ya-User-Ticket': self.user_ticket,
            'Content-Type': 'application/protobuf',
        }

        host = self.host + settings['update_url']

        memento_req = TReqChangeUserObjects()
        memento_req.ParseFromString(base64.b64decode(data))

        request = RTLogHTTPRequest(
            host,
            method='POST',
            body=memento_req.SerializeToString(),
            headers=headers,
            request_timeout=settings['request_timeout'],
            retries=settings['retries'],
            rt_log=self.rt_log,
            rt_log_label='memento_update',
        )

        begin = time.monotonic()
        try:
            response = yield _get_client(host).fetch(request)
            GlobalCounter.MEMENTO_UPDATE_REQUEST_OK_SUMM.increment()
            GlobalTimings.store(MEMENTO_UPDATE_REQUEST_HGRAM, response.response_time())
            return

        except Exception as exc:
            if isinstance(exc, HTTPError):
                if exc.code == 403:
                    GlobalCounter.MEMENTO_UPDATE_REQUEST_FORBIDDEN_SUMM.increment()
                    return
            else:
                self.logger.warning('memento error: {}'.format(exc))
            GlobalCounter.MEMENTO_UPDATE_REQUEST_FAIL_SUMM.increment()
            GlobalTimings.store(MEMENTO_UPDATE_REQUEST_HGRAM, time.monotonic() - begin)
            raise
