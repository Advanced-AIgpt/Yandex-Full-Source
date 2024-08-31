# coding: utf-8
from __future__ import absolute_import, unicode_literals

import json
import logging
import attr
import pytz
import urllib
import urlparse

from alice.megamind.protos.scenarios.response_pb2 import TScenarioCommitResponse
from alice.megamind.protos.scenarios.request_pb2 import TScenarioApplyRequest

from google.protobuf.json_format import ParseDict

from requests.exceptions import RequestException, ConnectionError
from uuid import UUID

from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting
from vins_core.utils.datetime import datetime_to_timestamp
from vins_core.utils.decorators import retry_on_exception
from vins_core.utils.json_util import MessageToDict
from vins_core.utils.logging import lazy_logging
from vins_core.utils.metrics import sensors
from vins_core.utils.network import is_reset_by_peer_exc
from vins_core.dm.request_events import VoiceInputEvent
from vins_core.dm.form_filler.models import Form

from personal_assistant.blocks import parse_block
from personal_assistant.bass_result import BassReportResult, BassSetupResult, BassFormInfo
from personal_assistant.intents import gen_forbidden_intents_experiment, EXP_FORBIDDEN_INTENTS


logger = logging.getLogger(__name__)


BASS_WARNING_STATUS_CODES = (502, 504)


def _get_bass_url_setting(env_variable_name, yenv):
    return get_setting('DEV_BASS_API_URL', default=get_setting(env_variable_name, yenv=yenv))


FAST_QUERY_BASS_API_URL = _get_bass_url_setting('FAST_QUERY_BASS_API_URL', yenv={
    'development': 'http://bass.hamster.alice.yandex.net/',
    'testing': 'http://bass-testing.n.yandex-team.ru/',
    'production': 'http://bass-prod-fast.yandex.net/',
})
FAST_BASS_QUERY = 'fast'


SLOW_QUERY_BASS_API_URL = _get_bass_url_setting('SLOW_QUERY_BASS_API_URL', yenv={
    'development': 'http://bass.hamster.alice.yandex.net/',
    'testing': 'http://bass-testing.n.yandex-team.ru/',
    'production': 'http://bass-prod.yandex.net/',
})
SLOW_BASS_QUERY = 'slow'


HEAVY_QUERY_BASS_API_URL = _get_bass_url_setting('HEAVY_QUERY_BASS_API_URL', yenv={
    'development': 'http://bass.hamster.alice.yandex.net/',
    'testing': 'http://bass-testing.n.yandex-team.ru/',
    'production': 'http://bass-prod-heavy.yandex.net/',
})
HEAVY_BASS_QUERY = 'heavy'
ANY_BASS_QUERY = 'any'
ALL_SUPPORTED_BALANCER_TYPES = [FAST_BASS_QUERY, SLOW_BASS_QUERY, HEAVY_BASS_QUERY]


@lazy_logging
def _dump_json(obj):
    if isinstance(obj, basestring):
        return obj

    return json.dumps(obj, ensure_ascii=False, indent=2)


class PersonalAssistantAPIError(Exception):
    pass


@attr.s
class TestUser(object):
    token = attr.ib()
    client_ip = attr.ib()
    uuid = attr.ib()
    tags = attr.ib()
    login = attr.ib()
    lock = attr.ib()


class PersonalAssistantAPI(BaseHTTPAPI):
    _urls = {
        'vins': 'vins',
        'setup': 'setup',
        'user-context': 'userContext',
        'version': 'version',
        'test_user': 'test_user',
        'megamind/protocol/vins/commit': 'megamind/protocol/vins/commit'
    }
    TIMEOUT = float(get_setting('BASS_TIMEOUT', 4.0))
    MAX_RETRIES = int(get_setting('BASS_MAX_RETRIES', 3))

    @staticmethod
    def _get_bass_url(req_info, balancer_type, path):
        url = urlparse.urljoin(PersonalAssistantAPI._get_url_prefix(req_info, balancer_type), path)
        srcrwrs = [
            ':'.join((key, value)) for key, value in req_info.srcrwr.iteritems() if key != 'BASS'
        ] if req_info else []
        return url + '?' + urllib.urlencode({'srcrwr': srcrwrs}, doseq=True)

    @staticmethod
    def _get_url_prefix(req_info, balancer_type):
        if req_info is not None:
            bass_url = req_info.srcrwr.get('BASS')
            if bass_url:
                if not (bass_url.startswith('http:') or bass_url.startswith('https:')):
                    bass_url = 'http://' + bass_url
                return bass_url

        if balancer_type == SLOW_BASS_QUERY:
            return SLOW_QUERY_BASS_API_URL
        if balancer_type == HEAVY_BASS_QUERY:
            return HEAVY_QUERY_BASS_API_URL
        return FAST_QUERY_BASS_API_URL

    def _post(self, req_info, type_, data, balancer_type, is_json=True,  **kwargs):
        if 'headers' not in kwargs:
            headers = {}
            if req_info is not None:
                if 'oauth_token' in req_info.additional_options:
                    headers['Authorization'] = (
                        'OAuth %s' % req_info.additional_options['oauth_token']
                    )
                if 'user_ticket' in req_info.additional_options:
                    headers['X-Ya-User-Ticket'] = req_info.additional_options['user_ticket']
                else:
                    logger.warning('No user ticket provided')
        else:
            headers = kwargs.pop('headers')

        joker_mocker_proxy = get_setting('JOKER_MOCKER_PROXY', default='', prefix='')
        if req_info is not None:
            if req_info.additional_options.get('joker'):
                headers['x-yandex-proxy-header-x-yandex-joker'] = req_info.additional_options['joker']
            if req_info.additional_options.get('joker_proxy'):
                joker_mocker_proxy = req_info.additional_options['joker_proxy']  # header has priority over env

            for k, v in req_info.proxy_header.items():
                headers[k] = v
        if joker_mocker_proxy:
            headers['x-yandex-via-proxy'] = joker_mocker_proxy

        @retry_on_exception(ConnectionError, pred=is_reset_by_peer_exc, retries=1)
        def make_json_request():
            return self.post(
                self._get_bass_url(req_info, balancer_type, self._urls[type_]),
                json=data,
                headers=headers,
                **kwargs
            )

        @retry_on_exception(ConnectionError, pred=is_reset_by_peer_exc, retries=1)
        def make_data_request():
            return self.post(
                self._get_bass_url(req_info, balancer_type, self._urls[type_]),
                data=data,
                headers=headers,
                **kwargs
            )

        try:
            resp = make_json_request() if is_json else make_data_request()
            sensors.inc_counter('bass_response', labels={'status_code': resp.status_code})
            return resp
        except RequestException as e:
            if e.response is not None and e.response.status_code in BASS_WARNING_STATUS_CODES:
                logger.warning('Failed request to BASS', exc_info=True)
            else:
                logger.error('Failed request to BASS', exc_info=True)
            raise PersonalAssistantAPIError

    def _validate_response_code(self, response):
        if response.status_code != 200:
            logger.log(
                logging.WARNING if response.status_code in BASS_WARNING_STATUS_CODES else logging.ERROR,
                'BASS returned status code %s', response.status_code,
                extra={'data': {
                    'response_content': response.content,
                    'status_code': response.status_code,
                }},
            )
            raise PersonalAssistantAPIError

    def _validate(self, response):
        self._validate_response_code(response)

        json = response.json()
        if 'error' in json:
            logger.error(
                'BASS returned error \'%s\'. Requested \'%s\'', json, response.request.body,
                extra={'data': {
                    'request_body': response.request.body,
                    'response_json': json,
                }},
            )
            error = json['error']
            error_str = error.get('msg', '') if isinstance(error, dict) else error
            raise PersonalAssistantAPIError(error_str)

        return json

    def get_version(self, req_info):
        resp = self._post(req_info, 'version', {}, balancer_type=FAST_BASS_QUERY,
                          headers={}, request_label='version')
        data = self._validate(resp)
        return data['svn_revision'].split('@')[-1]

    def save_value(self, req_info, uuid, key, value):
        assert isinstance(uuid, (basestring, UUID)), 'uuid should be instance of string or UUID'
        assert isinstance(key, basestring)

        data = {
            'uuid': str(uuid),
            'set': [
                {
                    'key': key,
                    'value': value,
                }
            ]
        }

        response = self._post(req_info, 'user-context', data, balancer_type=FAST_BASS_QUERY,
                              request_label='save-value')
        self._validate(response)

    def load_value(self, req_info, uuid, key):
        assert isinstance(uuid, (basestring, UUID)), 'uuid should be instance of string or UUID'
        assert isinstance(key, basestring)

        data = {
            'uuid': str(uuid),
            'get': [
                key
            ]
        }

        response = self._post(req_info, 'user-context', data, balancer_type=FAST_BASS_QUERY,
                              request_label='load-value')
        response_data = self._validate(response)

        if 'context' not in response_data:
            return None
        return response_data['context'][0]['value']

    @staticmethod
    def _gen_client_id(app_info):
        return '{app_id}/{app_version} ({device_manufacturer} {device_model}; {platform} {os_version})'.format(
            app_id=app_info.app_id,
            app_version=app_info.app_version,
            device_manufacturer=app_info.device_manufacturer,
            device_model=app_info.device_model,
            platform=app_info.platform,
            os_version=app_info.os_version,
        )

    @staticmethod
    def _convert_uuid_to_str(uuid):
        if uuid is None:
            return None
        return uuid.hex if isinstance(uuid, UUID) else str(uuid)

    def _get_meta(self, req_info, session, is_banned=None):
        if req_info.client_time.tzinfo is None:
            tz = str(pytz.UTC)
        else:
            tz = str(req_info.client_time.tzinfo)

        uuid = self._convert_uuid_to_str(req_info.uuid)
        device_id = self._convert_uuid_to_str(req_info.device_id)

        experiments = req_info.experiments.to_dict()
        forbidden_intents = gen_forbidden_intents_experiment(req_info)
        if forbidden_intents:
            experiments[EXP_FORBIDDEN_INTENTS] = forbidden_intents

        meta = dict(
            req_info.additional_options.get('bass_options', {}),
            config_patch=req_info.additional_options.get('config_patch', {}),
            uuid=uuid,
            device_id=device_id,
            dialog_id=req_info.dialog_id,
            has_image_search_granet=req_info.has_image_search_granet,
            request_id=req_info.request_id,
            rng_seed=req_info.rng_seed,
            sequence_number=req_info.sequence_number,
            hypothesis_number=req_info.utterance and req_info.utterance.hypothesis_number,
            end_of_utterance=req_info.utterance and req_info.utterance.end_of_utterance,
            tz=tz,
            epoch=datetime_to_timestamp(req_info.client_time),
            server_time_ms=req_info.additional_options.get('server_time_ms', 0),
            location=req_info.location,
            client_id=self._gen_client_id(req_info.app_info),
            lang=req_info.lang,
            user_lang=req_info.user_lang,
            experiments=experiments,
            utterance=req_info.utterance and req_info.utterance.text,
            laas_region=req_info.laas_region,
            device_state=req_info.device_state,
            voice_session=bool(req_info.voice_session),
            client_info=dict(
                app_id=req_info.app_info.app_id,
                app_version=req_info.app_info.app_version,
                device_manufacturer=req_info.app_info.device_manufacturer,
                device_model=req_info.app_info.device_model,
                platform=req_info.app_info.platform,
                os_version=req_info.app_info.os_version,
            ),
            client_features=dict(
                supported=req_info.additional_options.get('supported_features') or [],
                unsupported=req_info.additional_options.get('unsupported_features') or [],
            ),
            permissions=req_info.additional_options.get('permissions') or [],
            personal_data=req_info.personal_data,
            memento=req_info.memento,
            is_banned=is_banned
        )

        server_time_ms = req_info.additional_options.get('server_time_ms')
        if server_time_ms is not None:
            meta['request_start_time'] = 1000 * server_time_ms

        if req_info.utterance and req_info.utterance.payload:
            meta['utterance_data'] = req_info.utterance.payload

        if session is not None and session.get('pure_general_conversation'):
            meta['pure_gc'] = True

        has_config_options = req_info.additional_options.get('config_patch', {})
        pa_skills_url = get_setting('BASS_SKILLS_URL', default='')
        pa_avatars_host = get_setting('BASS_AVATARS_HOST', default='')
        need_config_patch = pa_skills_url or pa_avatars_host
        if not has_config_options and need_config_patch:
            meta['config_patch'] = {
                'Vins': {
                    'ExternalSkills': {
                    }
                }
            }
            if pa_skills_url:
                meta['config_patch']['Vins']['ExternalSkillsApi'] = {
                    'Host': pa_skills_url
                }
            if pa_avatars_host:
                meta['config_patch']['Vins']['ExternalSkills']['AvatarsHost'] = pa_avatars_host

        if isinstance(req_info.event, VoiceInputEvent):
            logger.debug("Attaching biometrics scores %s", req_info.event.biometrics_scores())
            meta['biometrics_scores'] = req_info.event.biometrics_scores()
            meta['biometry_classification'] = req_info.event.biometry_classification()
            meta['asr_utterance'] = req_info.event.asr_utterance()

        return meta

    @staticmethod
    def _serialize_form(form):
        # create FormInfo object from raw form data and serialize it
        return BassFormInfo.from_form(form).to_dict()

    @sensors.with_timer('bass_setup_time')
    def setup_forms(self, req_info, forms, balancer_type, bass_session_state=None, session=None, is_banned=None):
        if not all((form is None or isinstance(form, Form) for form in forms)):
            raise ValueError('each form must be either None or an instance of ' + Form.__module__ + '.' + Form.__name__)
        request = {
            'forms': [self._serialize_form(form) for form in forms],
            'meta': self._get_meta(req_info, session, is_banned=is_banned),
            'session_state': bass_session_state
        }
        logger.debug('BASS setup request %s', _dump_json(request))
        response = self._post(req_info, 'setup', request, balancer_type, request_label='setup')
        data = self._validate(response)
        logger.debug('BASS setup response %s', _dump_json(data))

        return BassSetupResult.from_dict(data)

    @sensors.with_timer('bass_response_time')
    def submit_form(
        self, req_info, form, action, balancer_type, bass_session_state=None, precomputed_data=None, session=None,
        is_banned=None
    ):
        if form is not None and not isinstance(form, Form):
            raise ValueError('form must be either None or an instance of ' + Form.__module__ + '.' + Form.__name__)

        request = precomputed_data or {}
        request.update({
            'form': form and self._serialize_form(form),
            'action': action,
            'meta': self._get_meta(req_info, session, is_banned=is_banned),
            'session_state': bass_session_state,
            'data_sources': req_info.data_sources
        })

        logger.debug('BASS report request %s', _dump_json(request))
        response = self._post(req_info, 'vins', request, balancer_type,
                              request_label='vins-{0}'.format(form.name) if form else 'vins')
        data = self._validate(response)
        logger.debug('BASS report response %s', _dump_json(data))

        form_info = data.get('form') and BassFormInfo.from_dict(data['form'])
        blocks = map(parse_block, data.get('blocks', []))

        return BassReportResult(form_info, blocks, data.get('session_state'))

    @sensors.with_timer('bass_commit_time')
    def proxy_commit(self, req_info, request, balancer_type):
        logger.debug('BASS commit request %s', request)
        post_request = ParseDict(request, TScenarioApplyRequest()).SerializeToString()
        response = self._post(req_info, 'megamind/protocol/vins/commit', post_request, balancer_type, is_json=False,
                              request_label='vins-commit')
        logger.debug('BASS commit response %s', response)
        self._validate_response_code(response)

        message = TScenarioCommitResponse.FromString(response.content)
        return MessageToDict(message)

    def _convert_response_to_test_user(self, response):
        response_json = self._validate(response)
        if 'result' in response_json:
            return TestUser(
                token=response_json['result']['token'] or '',
                client_ip=response_json['result']['client_ip'],
                uuid=response_json['result']['uuid'],
                tags=response_json['result']['tags'] or [],
                login=response_json['result']['login'],
                lock=response_json['result']['lock'],
            )

    def get_test_user(self, tags_list):
        """
        Parameters
        ----------
        tags_list : list
            Test User Parameters, example: ['has_home', 'has_work']
        Returns
        -------
            TestUser class instance

        """
        data = {"method": "GetUser", "args": {'tags': tags_list}}
        response = self._post(req_info=None, type_='test_user', data=data, balancer_type=None,
                              request_label='GetUser')
        return self._convert_response_to_test_user(response)

    def free_test_user(self, test_login):
        """
        Parameters
        ----------
        test_login : string
            Test User Parameters, example: 'test_login'
        """
        data = {"method": "FreeUser", "args": {'login': test_login}}
        self._post(req_info=None, type_='test_user', data=data, balancer_type=None,
                   request_label='FreeUser')
