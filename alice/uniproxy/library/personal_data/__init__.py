import json
import datetime
import tornado.gen

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, RTLogHTTPRequest, HTTPError
from alice.uniproxy.library.auth.blackbox import blackbox_client
from alice.uniproxy.library.auth.tvm2 import service_ticket_for_data_sync
from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter.uniproxy import DATASYNC_REQUEST_HGRAM
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.utils import conducting_experiment


DATASYNC_ADDRESSES_PREFIX = '/v2/personality/profile/addresses'
DATASYNC_KV_PREFIX = '/v1/personality/profile/alisa/kv'
DATASYNC_SETTINGS_PREFIX = '/v1/personality/profile/alisa/settings'


def _get_client(url):
    return QueuedHTTPClient.get_client_by_url(url)


class PersonalDataHelper:
    def __init__(self, system, request_info, rt_log, personal_data_response_future=None):
        self.session_id = system.session_id
        self.oauth_token = system.get_oauth_token()
        self.client_ip = system.client_ip
        self.client_port = system.client_port
        self.rt_log = rt_log
        self.uuid = request_info.get('uuid')
        self._log = Logger.get('.personaldata')
        self.bb_client = None
        self.create_clients()

        self.store_responses = conducting_experiment('context_load_diff', request_info)
        if self.store_responses:
            self.req_id = request_info.get('header', {}).get('request_id')
            self.responses_storage = system.responses_storage

        self._persdata_resp_fut = personal_data_response_future  # alternative (preferred) way to get response for `get_personal_data`

    def _get_prefix_handler_pairs(self, only_settings=False):
        if only_settings:
            return [
                (DATASYNC_SETTINGS_PREFIX, self._parse_settings_response)
            ]

        return [
            (DATASYNC_ADDRESSES_PREFIX, self._parse_addresses_response),
            (DATASYNC_KV_PREFIX, self._parse_kv_response)
        ]

    def create_clients(self):
        self.bb_client = blackbox_client()

    @tornado.gen.coroutine
    def _get_common_headers(self):
        headers = {'Content-Type': 'application/json; charset=utf-8'}
        try:
            headers['X-Ya-Service-Ticket'] = yield service_ticket_for_data_sync(rt_log=self.rt_log)
        except Exception as err:
            self.ERR('failed to get a service ticket:', err)
        return headers

    @tornado.gen.coroutine
    def _get_headers_ordered_collection(self):
        result = []
        common_headers = yield self._get_common_headers()

        if self.oauth_token and self.client_ip:
            headers = dict(common_headers)
            try:
                headers['X-Ya-User-Ticket'] = yield self.bb_client.ticket4oauth(
                    self.oauth_token,
                    self.client_ip,
                    self.client_port,
                    rt_log=self.rt_log,
                )
                result.append(headers)
            except Exception as err:
                self.ERR('failed to get a user ticket:', err)

        # Historically different strings were used as identificators of
        # unauthorized users. Adding them in the order of preference for write
        # operations.
        if self.uuid:
            headers = dict(common_headers)
            headers['X-Uid'] = "device_id:{}".format(self.uuid)
            result.append(headers)

            headers = dict(common_headers)
            headers['X-Uid'] = "uuid:{}".format(self.uuid)
            result.append(headers)

        return result

    # ad-hoc hashes (for 3 distinct requests)
    def _calc_request_hash(self, request):
        if 'X-Uid' in request.headers:
            if request.headers['X-Uid'].startswith('device_id'):
                return 'datasync_device_id'
            return 'datasync_uuid'
        return 'datasync'

    @tornado.gen.coroutine
    def _request_datasync(self, headers, items, raise_exception=False, store_response=False):
        url = '{}/v1/batch/request'.format(config['data_sync']['url'])
        for i in range(config['data_sync']['retries']):
            request = RTLogHTTPRequest(
                url,
                request_timeout=config['data_sync']['request_timeout'],
                method='POST',
                headers=headers,
                body=json.dumps({'items': items}).encode('utf-8'),
                rt_log=self.rt_log,
                rt_log_label='personal_data',
            )
            try:
                if store_response and self.req_id:
                    response = yield _get_client(url).fetch(request, raise_error=False)
                    if response.code // 100 != 2:
                        raise HTTPError(response.code, response.body, response.request)
                    self.responses_storage.store(self.req_id, self._calc_request_hash(request), response)
                else:
                    response = yield _get_client(url).fetch(request)

                GlobalCounter.DATASYNC_REQUEST_OK_SUMM.increment()
                GlobalTimings.store(DATASYNC_REQUEST_HGRAM, response.response_time())

                return response.text(), response.response_time()
            except Exception as err:
                if isinstance(err, HTTPError):
                    msg = err.text() or ''
                    self.ERR('request to data sync failed: {} {}'.format(err, msg))
                else:
                    self.ERR('request to data sync failed: {}'.format(err))
                e2 = err
        if store_response and self.req_id:
            self.responses_storage.store(self.req_id, self._calc_request_hash(request), response)
        GlobalCounter.DATASYNC_REQUEST_FAIL_SUMM.increment()
        if raise_exception:
            raise e2
        return None, None

    @tornado.gen.coroutine
    def _get_personal_data(self, headers, only_settings=False):
        prefix_handler_pairs = self._get_prefix_handler_pairs(only_settings)

        items = [{'method': 'GET', 'relative_url': prefix}
                 for (prefix, _) in prefix_handler_pairs]

        response, request_time = yield self._request_datasync(headers, items, store_response=self.store_responses)

        if only_settings:
            if not response:
                GlobalCounter.DATASYNC_SETTINGS_ERR_SUMM.increment()
            else:
                GlobalCounter.DATASYNC_SETTINGS_OK_SUMM.increment()
                GlobalTimings.store('datasync_settings_response', request_time)

        if not response:
            return  # an appropriate message should be already logged

        return self._unpack_batch_request(response, prefix_handler_pairs)

    def _unpack_batch_request(self, response, prefix_handler_pairs):
        try:
            response_json = json.loads(response)
        except Exception as err:
            self.ERR('failed to parse response from data sync:', err)
            return

        response_items = response_json.get('items', [])
        if len(response_items) != len(prefix_handler_pairs):
            if len(response_items) < len(prefix_handler_pairs):
                self.ERR(
                    'got unexpected number of response items: {} < {}'.format(
                        len(response_items), len(prefix_handler_pairs)
                    )
                )
                return

        result = []
        for response_item, (_, handler) in zip(response_items[:len(prefix_handler_pairs)], prefix_handler_pairs):
            result.extend(handler(response_item))

        return result

    @tornado.gen.coroutine
    def get_personal_data(self, only_settings=False):

        def _pack_personal_datas(datas):
            result = {}
            all_res_are_none = True
            for personal_data in datas:
                if personal_data is not None:
                    all_res_are_none = False
                if not personal_data:
                    continue  # an appropriate message should be already logged

                for key, value in personal_data:
                    if key not in result:
                        result[key] = value

            return result, all_res_are_none

        if self._persdata_resp_fut is not None:  # AppHost way
            try:
                # TContextLoadResponse of alice/cuttlefish/library/protos/context_load.proto
                response = yield tornado.gen.with_timeout(datetime.timedelta(seconds=10), self._persdata_resp_fut)
                if response is None:  # cancelled
                    return {}, True

                personal_data_collection = []
                prefix_handler_pairs = self._get_prefix_handler_pairs(only_settings)
                for field_name in ("DatasyncResponse", "DatasyncDeviceIdResponse", "DatasyncUuidResponse"):
                    if not response.HasField(field_name):
                        continue

                    http_response = getattr(response, field_name)  # THttpResponse of apphost/lib/proto_answers/http.proto
                    if http_response.StatusCode // 100 != 2:
                        continue

                    personal_data_collection.append(self._unpack_batch_request(http_response.Content, prefix_handler_pairs))

                ret = _pack_personal_datas(personal_data_collection)
                GlobalCounter.CLD_APPLY_PERSONAL_DATA_OK_SUMM.increment()
                return ret
            except Exception as exc:
                self.EXC(f"Failed to use personal data response from future (fallback to classic): {exc}")
                GlobalCounter.CLD_APPLY_PERSONAL_DATA_ERR_SUMM.increment()

        # classic way
        headers_collection = yield self._get_headers_ordered_collection()
        if not headers_collection:
            return None, None

        personal_data_collection = yield [self._get_personal_data(headers, only_settings) for headers in headers_collection]
        return _pack_personal_datas(personal_data_collection)

    @tornado.gen.coroutine
    def update_personal_data(self, directives):
        headers_collection = yield self._get_headers_ordered_collection()
        if not headers_collection:
            self.ERR('failed to get any headers')
            return

        headers = headers_collection[0]

        items = []
        for directive in directives:
            url = directive.get('key', '')
            value = directive.get('value')
            body = {'value': value} if url.startswith(DATASYNC_KV_PREFIX) else value
            body_s = json.dumps(body)
            items.append({
                'method': directive.get('method'),
                'relative_url': url,
                'body': body_s
            })

            if url.startswith(DATASYNC_ADDRESSES_PREFIX):
                GlobalTimings.store('datasync_addresses_upd_size_hgram', len(body_s))
            elif url.startswith(DATASYNC_KV_PREFIX):
                GlobalTimings.store('datasync_kv_upd_size_hgram', len(body_s))
            elif url.startswith(DATASYNC_SETTINGS_PREFIX):
                GlobalTimings.store('datasync_settings_upd_size_hgram', len(body_s))

        yield self._request_datasync(headers, items, raise_exception=True)

    @staticmethod
    def _parse_kv_response(response):
        result = []
        body = json.loads(response.get('body', '{}'))
        GlobalTimings.store('datasync_kv_get_size_hgram', len(response.get('body', '')))
        for item in body.get('items'):
            id = item.get('id', '')
            value = item.get('value')
            if id:
                result.append((DATASYNC_KV_PREFIX + '/' + id, value))
        return result

    @staticmethod
    def _parse_addresses_response(response):
        result = []
        body = json.loads(response.get('body', '{}'))
        GlobalTimings.store('datasync_addresses_get_size_hgram', len(response.get('body', '')))
        for item in body.get('items', []):
            id = item.get('address_id', '')
            if id:
                result.append((DATASYNC_ADDRESSES_PREFIX + '/' + id, item))
        return result

    @staticmethod
    def _parse_settings_response(response):
        result = []
        body = json.loads(response.get('body', '{}'))
        GlobalTimings.store('datasync_settings_get_size_hgram', len(response.get('body', '')))
        for item in body.get('items', []):
            id = item.get('id', '')
            result.append((id, item))
        return result

    def ERR(self, *args):
        try:
            self._log.error('{} PersonalDataHelper: {}'.format(self.session_id, args), rt_log=self.rt_log)
        except ReferenceError:
            pass

    def EXC(self, msg):
        try:
            self._log.exception(f"{self.session_id} PersonalDataHelper: {msg}", rt_log=self.rt_log)
        except ReferenceError:
            pass
