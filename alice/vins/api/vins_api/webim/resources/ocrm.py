# coding: utf-8
from __future__ import unicode_literals

import logging
import falcon
import os
import base64
import uuid

from jsonschema import validate as validate_json, ValidationError as SchemaValidationError

from alice.vins.api_helper.resources import ValidationError, set_json_response
from vins_api.webim.resources.webim import BaseWebimResource, DelayedResponseResource
from vins_api.webim.connectors.ocrm import OCRMConnector
from vins_api.webim.schemas_ocrm import ocrm_message as ocrm_schema
from vins_api.webim.primitives import OCRMDepartmentRedirect, TextMessage, Keyboard, UserInfo

from vins_core.utils.datetime import utcnow, timestamp_in_ms

logger = logging.getLogger(__name__)


def _format_ocrm_text_response_body(chat_id, text):
    return {
        "ServerMessageInfo": {
            "Timestamp": timestamp_in_ms(utcnow())
        },
        "Message": {
            "Plain": {
                "PayloadId": str(uuid.uuid4()),
                "ChatId": chat_id,
                "Text": {
                    "MessageText": text
                }
            }
        },
        "CustomFrom": {
            "DisplayName": "Робот Григорий"
        }
    }


def _add_kbd_to_response(response, kbd):
    response["Message"]["Plain"]["Text"]["ReplyMarkup"] = {"inline_keyboard": kbd.to_json()['buttons']}
    return response


class OCRMResource(BaseWebimResource):
    def __init__(self, *args, **kwargs):
        super(OCRMResource, self).__init__(*args, **kwargs)

    @classmethod
    def connector_cls(cls, *args, **kwargs):
        return OCRMConnector(*args, **kwargs)

    def _validate_request_schema(self, req):
        try:
            validate_json(req.context['body'], ocrm_schema)
        except SchemaValidationError as e:
            raise ValidationError(unicode(e))

    def _get_chat_id(self, req):
        return req.context['body']['Message']['Plain']['ChatId']

    def _get_version(self):
        return "1.0"

    def _get_app_id(self):
        return "ocrm.crm_bot"

    def _respond_ok(self, resp):
        set_json_response(resp, {}, falcon.HTTP_200)

    def _parse_message(self, req, data):
        text = data['Message']['Plain'].get('Text')
        if text is None or text.get('MessageText') is None or text.get('MessageText').startswith('/service'):
            return None
        return self._build_req_info(req, text.get('MessageText'))

    def _parse_request(self, req):
        user_info = UserInfo(
            req_info=self._build_req_info(req),
            chat_id=self._get_chat_id(req),
            uri=req.uri,
            req_id=req.context['reqid'],
            session_data={}
        )
        msg = self._parse_message(req, req.context['body'])
        return user_info, msg

    def _respond_with_result(self, req, resp, result, app_id):
        response = {
            'switch_operator': False,
            'messages': []
        }

        chat_id = self._get_chat_id(req)
        for msg in result.messages:
            if isinstance(msg, TextMessage):
                response['messages'].append(_format_ocrm_text_response_body(chat_id, msg.text))
            elif isinstance(msg, OCRMDepartmentRedirect):
                response['switch_operator'] = True
                response['category'] = msg.category

        if result.keyboard is not None and not result.keyboard.empty():
            if len(response['messages']) == 0:
                response['messages'].append(_format_ocrm_text_response_body(chat_id, ""))
            _add_kbd_to_response(response['messages'][-1], result.keyboard)

        if len(response['messages']) == 0:
            response['switch_operator'] = True

        set_json_response(resp, response, result.status_code)


class OCRMResource2(DelayedResponseResource):
    def __init__(self, *args, **kwargs):
        super(OCRMResource2, self).__init__(*args, **kwargs)

    @classmethod
    def connector_cls(cls, *args, **kwargs):
        return OCRMConnector(*args, **kwargs)

    # interfaces to implement
    def _validate_request_schema(self, req):
        try:
            validate_json(req.context['body'], ocrm_schema)
        except SchemaValidationError as e:
            raise ValidationError(unicode(e))

    def _parse_message(self, req, data):
        text = data['Message']['Plain'].get('Text')
        if text is None or text.get('MessageText') is None or text.get('MessageText').startswith('/service'):
            return None
        return self._build_req_info(req, text.get('MessageText'))

    def _parse_request(self, req):
        if req.get_header(self._settings.REQUID_HEADER, None) is None:
            raise RuntimeError(
                "OCRM API cannot work without \"{}\" header".format(self._settings.REQUID_HEADER))
        user_info = UserInfo(
            req_info=self._build_req_info(req),
            chat_id=self._get_chat_id(req),
            uri=req.uri,
            req_id=req.context['reqid'],
            session_data={}
        )
        msg = self._parse_message(req, req.context['body'])
        return user_info, msg

    def _get_chat_id(self, req):
        return req.context['body']['Message']['Plain']['ChatId']

    def _make_auth_headers(self):
        token = os.environ['OCRM_AUTH_TOKEN']
        return {
            'Authorization': 'Basic ' + base64.b64encode(token),
            'Content-Type': 'application/json'
        }

    def _get_base_url(self):
        return os.environ['OCRM_URL']

    def _get_chat_id_from_message(self, message):
        return message['chat_id']

    def _get_outgoung_auth_headers(self):
        if self._settings.REQUID_HEADER:
            return {self._settings.REQUID_HEADER: b"{}/{}".format(timestamp_in_ms(utcnow()), uuid.uuid4())}
        else:
            return {}

    def _prepare_message(self, response, msg, chat_id):
        headers = self._get_outgoung_auth_headers()
        if isinstance(msg, TextMessage):
            response.append({
                'url': '/v2/bot',
                'body': _format_ocrm_text_response_body(chat_id, msg.text),
                'headers': headers
            })
        elif isinstance(msg, Keyboard):
            response.append({
                'url': '/v2/bot',
                'body': _add_kbd_to_response(_format_ocrm_text_response_body(chat_id, ""), msg),
                'headers': headers
            })
        elif isinstance(msg, OCRMDepartmentRedirect):
            response.append({'url': '/{}/switch/operator'.format(chat_id), 'body': msg.to_json(), 'headers': headers})
        else:
            logger.info('Warning, cannot respond with {}'.format(msg))

    def _prepare_keyboard(self, response, msg, chat_id):
        headers = self._get_outgoung_auth_headers()
        if isinstance(msg, Keyboard):
            if len(response) == 0:
                logger.warning("Appending keyboard to empty result")
                response.append({
                    'url': '/v2/bot',
                    'body': _format_ocrm_text_response_body(chat_id, ""),
                    'headers': headers
                })
            _add_kbd_to_response(response[-1]['body'], msg)

    def _get_version(self):
        return "2.0"

    def _get_app_id(self):
        return "ocrm.crm_bot"

    def _respond_ok(self, resp):
        set_json_response(resp, {}, falcon.HTTP_200)
