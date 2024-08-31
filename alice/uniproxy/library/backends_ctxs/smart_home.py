import tornado.gen
import tornado.escape
import json
import base64

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPError
from alice.uniproxy.library.async_http_client import RTLogHTTPRequest
from alice.uniproxy.library.auth.tvm2 import service_ticket_for_smart_home
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import SMART_HOME_REQUEST_HGRAM
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import conducting_experiment

from alice.megamind.protos.common.iot_pb2 import TIoTUserInfo

from rtlog import null_logger


# for mocking
def _get_client(url):
    return QueuedHTTPClient.get_client_by_url(url)


def make_smart_home_response_from_proto(resp_proto, raw_proto=False):
    raw_data = {}

    if resp_proto.RawUserInfo:
        raw_data = json.loads(resp_proto.RawUserInfo)
        resp_proto.RawUserInfo = ''

    if raw_proto:
        return resp_proto, raw_data
    return base64.b64encode(resp_proto.SerializeToString()).decode('utf-8'), raw_data


def parse_smart_home_response(content, raw_proto=False):
    resp_proto = TIoTUserInfo()
    if content:
        resp_proto.ParseFromString(content)

    return make_smart_home_response_from_proto(resp_proto, raw_proto=raw_proto)


@tornado.gen.coroutine
def get_smart_home(get_user_ticket, device_id, rt_log=null_logger(), host=None, smarthomeuid=None, user_ticket=None, responses_storage=None, request_info={}, raw_proto=False):
    logger = Logger.get('.backends.smart_home')
    headers = {}
    try:
        ticket = yield service_ticket_for_smart_home(rt_log=rt_log)
        headers['X-Ya-Service-Ticket'] = ticket
    except Exception as exc:
        logger.exception('smart_home: fail getting service ticket' + str(exc))

    try:
        if user_ticket:
            headers['X-Ya-User-Ticket'] = user_ticket
        else:
            headers['X-Ya-User-Ticket'] = yield get_user_ticket()
    except Exception as exc:
        logger.exception('smart_home: can not get user-ticket: ' + str(exc))

    if headers.get('X-Ya-User-Ticket', None) is None:
        raise Exception('smart_home: user_ticket is None, request to IoT without user_ticket is pointless')

    if user_ticket:
        headers['X-Ya-User-Ticket'] = user_ticket

    if device_id:
        headers['X-Device-Id'] = tornado.escape.url_escape(device_id)

    if smarthomeuid:
        headers['X-Alice4business-Uid'] = tornado.escape.url_escape(str(smarthomeuid))

    headers['Accept'] = 'application/protobuf'

    settings = config['smart_home']
    if not host:
        host = settings['url']

    request = RTLogHTTPRequest(
        host,
        headers=headers,
        request_timeout=settings['request_timeout'],
        retries=settings['retries'],
        rt_log=rt_log,
        rt_log_label='get_smart_home',
        need_str=True,
    )

    try:
        req_id = None
        store_responses = conducting_experiment('context_load_diff', request_info)
        if store_responses:
            req_id = request_info.get('header', {}).get('request_id')

        if store_responses and req_id:
            response = yield _get_client(host).fetch(request, raise_error=False)

            responses_storage.store(req_id, 'quasar_iot', response)

            if response.code // 100 != 2:
                raise HTTPError(response.code, response.body, response.request)
        else:
            response = yield _get_client(host).fetch(request)

        resp, raw_data = parse_smart_home_response(response.body, raw_proto=raw_proto)

        GlobalCounter.SMART_HOME_REQUEST_OK_SUMM.increment()
        GlobalTimings.store(SMART_HOME_REQUEST_HGRAM, response.response_time())

        return resp, raw_data
    except Exception as exc:
        if isinstance(exc, HTTPError):
            if exc.code == 401:
                GlobalCounter.SMART_HOME_REQUEST_UNAUTHORIZED_SUMM.increment()
                if raw_proto:
                    return TIoTUserInfo(), {}
                return "", {}
            elif exc.code == 403:
                GlobalCounter.SMART_HOME_REQUEST_FORBIDDEN_SUMM.increment()
                if raw_proto:
                    return TIoTUserInfo(), {}
                return "", {}
        else:
            logger.error('smart_home error: {}'.format(exc))
        GlobalCounter.SMART_HOME_REQUEST_FAIL_SUMM.increment()
        raise
