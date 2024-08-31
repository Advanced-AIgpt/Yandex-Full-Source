# coding: utf-8

from __future__ import unicode_literals

import logging
import os
import time
import random
from uuid import uuid4 as gen_request_id

from ylog.context import LogContext

from vins_core.dm.response import UniproxyActionDirective
from vins_core.dm.request_events import VoiceInputEvent
from vins_core.utils.config import get_bool_setting, get_setting
from vins_core.utils.metrics import sensors
from rtlog import AsJson

from alice.vins.api_helper.resources import (
    set_json_response,
    dump_request_for_log,
    dump_response_for_log,
    status2code,
)

from vins_api.common.resources import BaseConnectedAppResource
from vins_api.speechkit.resources.common import parse_sk_request, http_status_code_for_response

from vins_api.speechkit.schemas import request_schema
from vins_api.speechkit.connectors.speechkit import SpeechKitConnector
from vins_api.speechkit.session import SKSessionStorage

logger = logging.getLogger(__name__)


class SKResource(BaseConnectedAppResource):
    storage_cls = SKSessionStorage

    @classmethod
    def connector_cls(cls, *args, **kwargs):
        listen_by_default = get_bool_setting('SPEECHKIT_LISTEN_BY_DEFAULT')
        return SpeechKitConnector(
            listen_by_default=listen_by_default,
            *args, **kwargs
        )

    def get_request_id(self, data):
        return data['header'].get('request_id') or str(gen_request_id())

    def on_post(self, req, resp, app_id):
        start_time = time.time()

        rng_seed_salt = get_setting('RANDOM_SEED_SALT', default='')
        (req_info, req_data) = parse_sk_request(req, request_schema, self._settings.EXPERIMENTS, rng_seed_salt=rng_seed_salt)

        with LogContext(request_id=str(req_info.request_id),
                        uuid=str(req_info.uuid),
                        device_id=str(req_info.device_id),
                        worker_pid=os.getpid()):
            logger.debug('Received request: %(request)s', {'request': AsJson(dump_request_for_log(req_data))})

            with sensors.timer('view_handle_request_time'):
                with sensors.labels_context({'app_id': req_info.app_info.app_id}):
                    app = self.get_or_create_connected_app(app_id)

                    # Regarding the random seed:
                    # NOTE 1: Avoid this in a multithreaded execution (at the moment there is none)
                    # NOTE 2: Yes this affects all the other parts where python random singleton object is used, but there should be no harm
                    random.seed(req_info.rng_seed)

                    result = app.handle_request(req_info)

            status_code = http_status_code_for_response(result)

            with sensors.timer('view_serialize_time', labels={'status_code': status2code(status_code)}):
                resp_data = self.serialize_result(req_info, result)
                set_json_response(resp, resp_data, status_code)
                resp.set_header('X-Yandex-Vins-OK', 'true')

            logger.debug('Speechkit Api response %s', dump_response_for_log(resp))
        logger.info('Request %s start_time: %f end_time: %f', req_info.request_id, start_time, time.time())

    @staticmethod
    def serialize_result(req_info, vins_response):
        if req_info.voice_session is not None:
            voice_session = req_info.voice_session
        else:
            voice_session = isinstance(req_info.event, VoiceInputEvent)

        resp_data = {
            'header': {
                'request_id': req_info.request_id,
                'response_id': gen_request_id(),
                'dialog_id': req_info.dialog_id,
                'prev_req_id': req_info.prev_req_id,
                'sequence_number': req_info.sequence_number,
            },
            'response': {
                'card': None,
                'cards': [],
                'suggest': None,
                'directives': [],
                'meta': [],
                'features': {},
                'experiments': req_info.experiments.to_dict()
            },
            'voice_response': {
                'output_speech': None,
                'should_listen': voice_session and vins_response.should_listen,
                'directives': [],
            }
        }
        if vins_response.voice_text and (voice_session or vins_response.force_voice_answer):
            resp_data['voice_response']['output_speech'] = {
                'type': 'simple',
                'text': vins_response.voice_text,
            }
        if vins_response.cards:
            resp_data['response']['cards'] = [c.to_dict() for c in vins_response.cards]
            resp_data['response']['card'] = resp_data['response']['cards'][0]
        if vins_response.suggests:
            resp_data['response']['suggest'] = {
                'items': [s.to_dict() for s in vins_response.suggests]
            }
        if vins_response.directives:
            resp_data['voice_response']['directives'] = [
                d.to_dict() for d in vins_response.directives if isinstance(d, UniproxyActionDirective)
                ]
            resp_data['response']['directives'] = [
                d.to_dict() for d in vins_response.directives if not isinstance(d, UniproxyActionDirective)
                ]
        meta = vins_response.get_meta()
        if meta:
            resp_data['response']['meta'] = meta
        if vins_response.sessions:
            resp_data['sessions'] = vins_response.sessions
        if vins_response.special_buttons:
            resp_data['response']['special_buttons'] = [sp.to_dict() for sp in vins_response.special_buttons]
        if vins_response.features:
            resp_data['response']['features'] = {k: v.to_dict() for k, v in vins_response.features.iteritems()}

        if vins_response.autoaction_delay_ms is not None:
            resp_data['response']['autoaction_delay_ms'] = vins_response.autoaction_delay_ms

        if vins_response.megamind_actions:
            resp_data['response']['megamind_actions'] = {k: v for k, v in vins_response.megamind_actions}

        return resp_data
