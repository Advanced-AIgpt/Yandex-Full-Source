# coding: utf-8

import functools
import json

from alice.acceptance.modules.request_generator.lib import helpers
from alice.acceptance.modules.request_generator.lib import uniproxy


class CtxValidationException(Exception):
    pass


class AliceSessionBuilder:
    _request_generator = None
    _requests = None

    @staticmethod
    def validate_init_ctx(init_ctx):
        def _message(key, expected_types, actual_type):
            return 'Invalid "%s" argument type, %r expected, "%s" actual' % (key, expected_types, actual_type)

        def _run_assertion(key, types_):
            if not init_ctx.get(key):
                return
            assert isinstance(init_ctx[key], types_), _message(key, types_, type(init_ctx[key]))

        try:
            _run_assertion('process_id', str)
            _run_assertion('oauth_token', str)
            _run_assertion('experiments', str)
            _run_assertion('fetcher_mode', str)
            _run_assertion('retry_profile', str)
            _run_assertion('additional_options', dict)
            _run_assertion('deep_mode', bool)
            _run_assertion('downloader_flags', (str, list))
            _run_assertion('filter_experiments', list)
        except AssertionError as e:
            raise CtxValidationException(str(e))

    def __init__(self, init_ctx: dict):
        self.validate_init_ctx(init_ctx)
        if isinstance(init_ctx.get('downloader_flags'), str):
            init_ctx['downloader_flags'] = json.loads(init_ctx['downloader_flags'])
            assert isinstance(init_ctx['downloader_flags'], list), \
                '"downloader_flags" type "list" expected, actual "%s"' % (type(init_ctx['downloader_flags']))

        if init_ctx.get('experiments'):
            init_ctx['experiments'] = helpers.parse_experiments_from_options(init_ctx['experiments'])
        self._request_generator = functools.partial(uniproxy.mapper, deep_mode=True, **init_ctx)
        self._requests = []

    def new_session(self):
        self._requests = []

    def next_request(self, request_data, prev_response=None):
        if prev_response is None:
            self.new_session()
        else:
            assert len(self._requests) > 0, 'No requests in session'
            self.patch(request_data, self._requests[-1], prev_response)
        request = next(self._request_generator(request_data))
        self._requests.append(request)
        return request

    @staticmethod
    def patch(next_request, prev_request=None, prev_response=None):
        # NOTE: patch session, uuid, device_state etc here
        if prev_response and prev_response.get(b'VinsResponse'):
            vins_response = json.loads(prev_response[b'VinsResponse'])
            next_request[b'session'] = vins_response['directive']['payload']['sessions']['']
        if prev_request:
            next_request[b'uuid'] = prev_request[b'Uuid']
            next_request[b'x-rtlog-token'] = json.loads(prev_request[b'Headers'])['x-rtlog-token']
