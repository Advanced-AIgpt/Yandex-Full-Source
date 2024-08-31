# coding: utf-8

from __future__ import unicode_literals

import logging
import json
import pytz

from copy import copy
from requests import HTTPError

from vins_core.utils.config import get_setting
from vins_core.utils.datetime import datetime_to_timestamp
from vins_core.ext.base import BaseHTTPAPI
from vins_core.dm.response import Button, Card, ActionDirective
from vins_core.dm.request_events import VoiceInputEvent, TextInputEvent
from vins_core.dm.request import AppInfo, ReqInfo, Experiments
from vins_core.utils.misc import gen_uuid_for_tests

logger = logging.getLogger(__name__)


class SpeechKitHTTPAPI(BaseHTTPAPI):
    PA_DEFAULT_HOST = get_setting('PA_HOST', 'http://vins-int.tst.voicetech.yandex.net/speechkit/app/pa/')
    TIMEOUT = 4.0

    def __init__(self, vins_url=None, timeout=5.0, **kwargs):
        super(SpeechKitHTTPAPI, self).__init__(timeout=timeout, **kwargs)
        self._vins_url = vins_url or self.PA_DEFAULT_HOST

    def handle_request(self, req_info, response, **kwargs):
        self.deserialize_result(self._call_sk_api(req_info), response)

    @staticmethod
    def make_request(req_info):
        payload = {
            'header': {
                'request_id': str(req_info.request_id),
            },
            'application': {
                'app_id': req_info.app_info.app_id,
                'app_version': req_info.app_info.app_version,
                'os_version': req_info.app_info.os_version,
                'platform': req_info.app_info.platform,
                'uuid': str(req_info.uuid),
                'lang': req_info.lang,
                'client_time': req_info.client_time.strftime('%Y%m%dT%H%M%S'),
                'timezone': req_info.client_time.tzinfo.zone if req_info.client_time.tzinfo else pytz.utc.zone,
                'timestamp': str(datetime_to_timestamp(req_info.client_time)),
            },
            'request': {
                'additional_options': req_info.additional_options,
                'event': req_info.event.to_dict(),
                'experiments': req_info.experiments.to_dict(),
                'reset_session': req_info.reset_session,
            }
        }
        if req_info.app_info.device_manufacturer and req_info.app_info.device_model:
            payload['application']['device_manufacturer'] = req_info.app_info.device_manufacturer
            payload['application']['device_model'] = req_info.app_info.device_model

        if req_info.voice_session is not None:
            payload['request']['voice_session'] = req_info.voice_session
        if req_info.location:
            payload['request']['location'] = req_info.location

        if req_info.device_state:
            payload['request']['device_state'] = req_info.device_state
        else:
            payload['request']['device_state'] = {}

        if req_info.dialog_id:
            payload['header']['dialog_id'] = req_info.dialog_id

        if req_info.session is not None:
            payload['session'] = req_info.session

        return payload

    @staticmethod
    def make_headers(req_info):
        headers = dict()
        if req_info.srcrwr:
            headers['x-srcrwr'] = ';'.join(['%s=%s' % (k, v) for k, v in req_info.srcrwr.items()])
        if req_info.rtlog_token:
            headers['x-rtlog-token'] = req_info.rtlog_token
            headers['x-yandex-req-id'] = req_info.rtlog_token
        return headers

    def _call_sk_api(self, req_info):
        payload = self.make_request(req_info)
        headers = self.make_headers(req_info)
        logger.info('Sending request to speechkit api: %s', json.dumps(payload, ensure_ascii=False, indent=2))
        res = self.post(self._vins_url, json=payload, headers=headers, request_label=req_info.request_label)
        try:
            if res.headers.get('X-Yandex-Vins-OK') != 'true':
                res.raise_for_status()
            result_json = res.json()
            resp_data = json.dumps(result_json, ensure_ascii=False, indent=2)
            logger.info('Got response from speechkit api: %s', resp_data)
            return result_json
        except HTTPError:
            logger.warning('Got failed response from speechkit api: %s', res.text)
            raise

    @staticmethod
    def deserialize_result(api_response, response):
        if api_response['voice_response']:
            if api_response['voice_response'].get('output_speech', {}):
                response.voice_text = api_response['voice_response'].get('output_speech', {}).get('text')
            response.should_listen = api_response['voice_response']['should_listen']

        response.directives = [ActionDirective.from_dict(d) for d in api_response['response']['directives']]
        response.cards = [Card.from_dict(c) for c in api_response['response'].get('cards', [])]
        suggest = api_response['response'].get('suggest', {})
        if suggest:
            response.suggests = [Button.from_dict(d) for d in suggest.get('items', [])]
        response.special_buttons = [Button.from_dict(c) for c in api_response['response'].get('special_buttons', [])]
        response.sessions = api_response.get('sessions', {})

    @property
    def url(self):
        return self._vins_url


class SpeechKitTestAPI(SpeechKitHTTPAPI):
    DEFAULT_LOCATION = {
        'lon': 37.587937,
        'lat': 55.733771
    }  # Yandex Office
    DEFAULT_APPINFO = AppInfo(
        app_id='com.yandex.vins.tests',
        app_version='0.0.1',
        os_version='1',
        platform='unknown',
    )

    def __init__(self, bass_url=None, req_wizard_url=None, as_voice=False, app_info=None, **kwargs):
        super(SpeechKitTestAPI, self).__init__(**kwargs)
        self._as_voice = as_voice
        self._bass_url = bass_url
        self._req_wizard_url = req_wizard_url
        self._app_info = app_info

    @staticmethod
    def _response_to_str(response):
        return '\n'.join(c['text'] for c in response['response']['cards'])

    def request(
        self, uuid, dt, event, location=None, experiments=None, lang='ru-RU',
        app_info=None, string_response=True, device_state=None, additional_options=None,
        dialog_id=None, reset_session=False, ensure_purity=False, request_label=None,
        session=None
    ):
        if additional_options:
            additional_options = copy(additional_options)
        else:
            additional_options = {}

        srcrwr = dict()
        if self._req_wizard_url:
            srcrwr['ReqWizard'] = self._req_wizard_url
        if self._bass_url:
            srcrwr['BASS'] = self._bass_url

        request = ReqInfo(
            request_id=str(gen_uuid_for_tests()),
            client_time=dt,
            lang=lang,
            uuid=uuid,
            utterance=event.utterance,
            reset_session=reset_session,
            location=location or self.DEFAULT_LOCATION,
            app_info=app_info or self._app_info or self.DEFAULT_APPINFO,
            additional_options=additional_options,
            event=event,
            experiments=Experiments(experiments),
            voice_session=True,
            device_state=device_state or {},
            dialog_id=dialog_id,
            srcrwr=srcrwr,
            ensure_purity=ensure_purity,
            request_label=request_label,
            session=session
        )

        response = self._call_sk_api(request)
        return self._response_to_str(response) if string_response else response

    def say(
        self, uuid, dt, utterance, location=None, experiments=None, lang='ru-RU',
        app_info=None, string_response=True, device_state=None, dialog_id=None,
        additional_options=None, reset_session=False
    ):
        if self._as_voice:
            event = VoiceInputEvent.from_utterance(utterance)
        else:
            event = TextInputEvent(utterance)

        self.request(
            uuid, dt, event, location, experiments, lang, app_info, string_response,
            device_state, additional_options, dialog_id, reset_session
        )
