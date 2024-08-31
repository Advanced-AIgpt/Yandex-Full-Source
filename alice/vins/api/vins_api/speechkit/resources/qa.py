# coding: utf-8

from __future__ import unicode_literals

import logging
import json

import falcon
from google.protobuf import json_format

from alice.vins.api_helper.resources import set_json_response, set_protobuf_response
from vins_api.common.resources import BaseConnectedAppResource
from vins_api.speechkit.schemas import request_schema
from vins_api.speechkit.resources.common import parse_sk_request, http_status_code_for_response

from vins_core.common.sample import Sample
from vins_core.app_utils import get_nlu_result
from vins_core.utils.metrics import sensors


logger = logging.getLogger(__name__)


class NLUResource(BaseConnectedAppResource):
    use_dummy_storages = True

    def _handle_request(self, app, req_info, true_intent, pred_intent, prev_intent):
        response = {}
        if req_info.utterance.text:
            semantic_frames = get_nlu_result(
                app, req_info.utterance, req_info, prev_intent=prev_intent
            )[1].semantic_frames
            for semantic_frame in semantic_frames:
                semantic_frame['intent_name'] = app.try_rename_intent(semantic_frame['intent_name'], 'pred_intent')
            response['semantic_frames'] = semantic_frames
        response['true_intent'] = app.try_rename_intent(true_intent, 'true_intent')
        if pred_intent:
            response['pred_intent'] = app.try_rename_intent(pred_intent, 'pred_intent')
        return response

    def on_post(self, req, resp, app_id, **kwargs):
        (req_info, req_data) = parse_sk_request(req, None, self._settings.EXPERIMENTS)
        resp_data = self._handle_request(
            app=self.get_or_create_connected_app(app_id).vins_app,
            req_info=req_info,
            true_intent=req_data['request'].get('true_intent'),
            pred_intent=req_data['request'].get('pred_intent'),
            prev_intent=req_data['request'].get('prev_intent')
        )
        set_json_response(resp, resp_data)


class FeaturesResource(BaseConnectedAppResource):
    @sensors.with_timer('qa_api_features_response_time')
    def on_post(self, req, resp, app_id, **kwargs):
        (req_info, req_data) = parse_sk_request(req, request_schema, self._settings.EXPERIMENTS)
        with sensors.timer('qa_api_features_vins_response_time'):
            app = self.get_or_create_connected_app(app_id).vins_app
            vins_response = None
            if req.path.startswith('/qa/%s/features_light' % app_id):
                logger.debug('Light features request path: %s', req.path)
                (features, _) = app.handle_features(req_info, only_sample_features=True)
            else:
                logger.debug('Heavy features request path: %s', req.path)
                (features, vins_response) = app.handle_features(req_info)

        tags = req_info.additional_options.get('nlu_tags', [])
        old_sample = features.sample
        new_sample = Sample(old_sample.tokens, old_sample.utterance, tags)
        features.sample = new_sample

        status_code = falcon.HTTP_200
        if vins_response:
            status_code = http_status_code_for_response(vins_response)
        if req.get_param('_json') or req.get_header('Accept') == b'application/json':
            with sensors.timer('qa_api_features_protobuf_serialize', labels={'format': 'json'}):
                resp_obj = features.to_pb()
                json_obj = json.dumps(json_format.MessageToDict(resp_obj, including_default_value_fields=True), encoding='utf8')
                set_json_response(resp, json_obj, status_code, serialize_data=False)
        else:
            with sensors.timer('qa_api_features_protobuf_serialize', labels={'format': 'string'}):
                resp_obj = features.to_pb()
                resp_data = resp_obj.SerializeToString()
                set_protobuf_response(resp, resp_data, status_code)
