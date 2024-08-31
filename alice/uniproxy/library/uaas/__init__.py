import base64
import json

from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import ignore_exceptions, rtlog_child_activation
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format

from laas.lib.ip_properties.proto.ip_properties_pb2 import TIpProperties

from tornado.httpclient import AsyncHTTPClient, HTTPRequest
from urllib.parse import urlencode


def _count_flags_impl(flags_provider_name, call_source, flags_count):
    if not (call_source in ["zalipanie", "sync_state", "set_state"]):
        if not (call_source == "context_load" and flags_provider_name == "flags_json"):
            call_source = "invalid_call_source"

    if flags_count > 0:
        GlobalCounter.increment_counter(flags_provider_name, call_source, "nonempty")
        GlobalCounter.increment_counter(flags_provider_name, call_source, "total_count", d=flags_count)
    else:
        GlobalCounter.increment_counter(flags_provider_name, call_source, "empty")


class FlagsJsonResponse:
    def __init__(self, response_json, rt_log=None, call_source=None):
        self.rt_log = rt_log
        self._log = Logger.get('.flags_json')
        self._log.debug('flags_json response response_json={}'.format(response_json))

        response = self._parse_raw_response(response_json)

        if self.rt_log:
            self.rt_log.error('Flags.json ({}) response: {}'.format(call_source, response))

        self.__exp_config_version = response.get('exp_config_version')
        self._all_test_ids = list(map(int, response.get('all', {}).get('TESTID', [])))
        self._reqid = response.get('reqid')

        self._exp_boxes = response.get('exp_boxes')
        self._exp_flags = response.get('all', {}).get('CONTEXT', {}).get('MAIN', {})

        self._count_flags(call_source=call_source)

    def __str__(self):
        return 'exp_config_version={} exp_boxes={} exp_flags={}'.format(
            self.__exp_config_version,
            self._exp_boxes,
            json.dumps(self._exp_flags),
        )

    def _parse_raw_response(self, response_json) -> dict:
        if isinstance(response_json, dict):
            return response_json

        elif isinstance(response_json, str) or isinstance(response_json, bytes):
            try:
                return json.loads(response_json)
            except:
                self._log.warning(
                    'Unable to parse flags_json response: "{}"'.format(response_json),
                )
        else:
            self._log.warning(
                'Invalid format of flags_json response: {}({})'.format(type(response_json), response_json),
            )

        return dict()

    def _get_flags(self, handlers):
        return self._merge_flags([self._exp_flags.get(handler) for handler in handlers])

    def get_all_flags(self):
        return self._get_flags(['VOICE', 'ASR'])

    def get_voice_exps(self):
        return self._get_flags(['VOICE'])

    def get_ab_config(self, backend):
        return json.dumps({
            backend: {
                'flags': list(self._get_flags([backend]).keys()),
                'boxes': self._exp_boxes,
            }
        })

    def get_all_test_id(self):
        return self._all_test_ids

    def _merge_flags(self, exp_contexts):
        flags = dict()
        for exp_ctx in exp_contexts:
            # fix exception on experiments without CONTEXT subfield ASR or VOICE  (as example testid=325417)
            if isinstance(exp_ctx, dict) and ('flags' in exp_ctx):
                flags.update(safe_experiments_vins_format(exp_ctx['flags'], self._log.warn))
        return flags

    def _count_flags(self, call_source):
        all_flags = self.get_all_flags()
        _count_flags_impl("flags_json", call_source, len(all_flags))


class FlagsProviderClientBase:
    def __init__(
        self, url, on_result, on_error,
        ip,
        uuid,
        icookie,
        cookie=None,
        user_agent=None,
        yandex_env=None,
        tests_ids=None,
        app_info=None,
        device_id=None,
        rt_log=None,
        request_logger=None,
        puid=None,
        staff_login=None,
        no_tests=None,
        call_source=None,
        **kwargs
    ):
        """
            yandex_env = ('production' | 'preproduction' | 'testing')
            for timeout use HTTPRequest params/kwargs (float seconds): connect_timeout, request_timeout
        """

        self.on_success_cb = on_result
        self.on_error_cb = on_error
        self.client = None
        self.rt_log = rt_log
        self._log = self._make_logger()
        self._request_logger = request_logger
        self._call_source = call_source

        if tests_ids:
            yandex_env = 'testing'  # for getting from uaas flags only for given test ids

        cgi = {
            'uuid': uuid,
        }

        if device_id:
            cgi['deviceid'] = device_id

        if no_tests:
            cgi["no-tests"] = no_tests

        headers = {}

        if cookie:
            headers['Cookie'] = cookie

        if puid:
            headers['X-Yandex-Puid'] = puid

        properties = TIpProperties()
        properties.IsYandexStaff = bool(staff_login)
        properties = properties.SerializeToString()
        properties = base64.b64encode(properties).decode('utf-8')
        headers['X-Ip-Properties'] = properties

        if icookie:
            headers['X-Yandex-ICookie'] = icookie

        if yandex_env:
            headers['X-Yandex-UAAS'] = yandex_env

        if user_agent:
            headers['User-Agent'] = user_agent
        else:
            headers['User-Agent'] = 'uniproxy'  # dirty hack for quickfix USEREXP-5968

        if app_info:
            headers['X-Yandex-AppInfo'] = app_info

        url += '?' + urlencode(cgi)
        if tests_ids:

            # multiple test-ids are separeted by '_':
            # https://wiki.yandex-team.ru/jandekspoisk/kachestvopoiska/abt/uaas/#kakjamogupopastvtrebuemyevyborki
            url += '&test-id=' + '_'.join(map(str, tests_ids))

        with rtlog_child_activation(self.rt_log, self.RTLOG_ACTIVATION_NAME, finish=False) as token:
            self._rtlog_activation_token = token

            self.create_client(url)
            if self.rt_log:
                self.rt_log.error(
                    'Request to flags.json ({}) url={} headers={}'.format(self._call_source, url, headers)
                )
            self._log.warning('run request to UaaS url={} headers={}'.format(url, headers), rt_log=self.rt_log)
            request = HTTPRequest(url=url, method='GET', headers=headers, **kwargs)
            self.client.fetch(request, self.on_result)

    # pylint: disable=unused-argument
    def create_client(self, url):  # separate method for mocking self.client
        self.client = AsyncHTTPClient(force_instance=True)

    def close(self):
        if self.client is not None:
            self.client.close()
            self.client = None

    def _on_error(self, error_message):
        if self._rtlog_activation_token is not None:
            self._log.error('{}.on_error: {}'.format(self.__class__.__name__, error_message), rt_log=self.rt_log)
            self.rt_log.log_child_activation_finished(self._rtlog_activation_token, False)
        self.on_error_cb(error_message)

    def _on_success(self, *args, **kwargs):
        if self._rtlog_activation_token is not None:
            self.rt_log.log_child_activation_finished(self._rtlog_activation_token, True)
        self.on_success_cb(*args, **kwargs)


class FlagsJsonClient(FlagsProviderClientBase):
    RTLOG_ACTIVATION_NAME = "flags_json"

    def _make_logger(self):
        return Logger.get('.flags_json')

    # result/error callbacks usually use weakref of unisystem, we don't want to see tracebacks if it's deleted
    @ ignore_exceptions(ReferenceError)
    def on_result(self, result):
        GlobalCounter.increment_error_code("flags_json", result.code)
        if self._request_logger is not None:
            self._request_logger({
                "request": {
                    "url": result.request.url,
                    "headers": dict(result.request.headers)
                },
                "response": {
                    "code": result.code,
                    "headers": str(result.headers),
                }
            })

        if result.error:
            msg = result.error
            if result.body:
                body = result.body.decode('utf-8')
                msg = '{}: {}'.format(msg, body)
            self._on_error(msg)
        else:
            try:
                resp = FlagsJsonResponse(result.body, self.rt_log, call_source=self._call_source)
            except Exception as exc:
                GlobalCounter.FLAGS_JSON_BAD_RESPONSE_DATA_SUMM.increment()
                self._log.exception(exc, rt_log=self.rt_log)
                self._on_error('can not create FlagsJsonResponse: ' + str(exc))
                self.close()
                return
            self._on_success(resp)
        self.close()
