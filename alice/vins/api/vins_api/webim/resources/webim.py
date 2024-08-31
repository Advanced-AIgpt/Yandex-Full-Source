# coding: utf-8
from __future__ import unicode_literals

import logging
import falcon
import uuid
import os
import zlib
from datetime import datetime
import pytz
import copy
import requests
from vins_core.utils.data import to_json_str
from requests.exceptions import Timeout
from jsonschema import validate as validate_json, ValidationError as SchemaValidationError
from urlparse import parse_qs
import json
import re
import time

from alice.vins.api_helper.resources import ValidationError, set_json_response
from vins_core.dm.request import AppInfo, ReqInfo, Experiments
from vins_core.dm.request_events import TextInputEvent, SuggestedInputEvent, ImageInputEvent
from vins_core.common.utterance import Utterance
from vins_api.webim.schemas import webim_schema as webim_schema_v1
from vins_api.webim.schemas_v2 import webim_schema as webim_schema_v2
from vins_api.common.resources import BaseConnectedAppResource
from vins_api.speechkit.session import SKSessionStorage
from vins_api.webim.connectors.webim import WebimConnector
from vins_api.webim.primitives import TextMessage, FileMessage, Keyboard, OperatorRedirect, \
    DepartmentRedirect, CloseChat, UserInfo
from vins_api.webim.crypto import Decryptor

logger = logging.getLogger(__name__)


class BaseWebimResource(BaseConnectedAppResource):
    storage_cls = SKSessionStorage

    def __init__(self, *args, **kwargs):
        super(BaseWebimResource, self).__init__(*args, **kwargs)

    @classmethod
    def connector_cls(cls, *args, **kwargs):
        return WebimConnector(*args, **kwargs)

    # interfaces to implement
    def _validate_request_schema(self, req):
        raise NotImplementedError

    def _parse_request(self, req):
        raise NotImplementedError

    def _get_chat_id(self, req):
        raise NotImplementedError

    def _respond_with_result(self, req, resp, result, app_id):
        raise NotImplementedError

    def _get_version(self):
        raise NotImplementedError

    def _get_app_id(self):
        raise NotImplementedError

    def _modify_response(self, resp):
        pass

    def _respond_ok(self, resp):
        pass

    def _get_experiments(self, req):
        qs = req.query_string
        query = parse_qs(qs)

        # regular experiments
        exp_raw = query.get('experiments', [])
        experiments = []
        for exp in exp_raw:
            try:
                experiments += json.loads(exp)
            except ValueError:
                experiments += exp.split(',')

        # "timed" experiments
        # example: timed_exp=lt1581927994__disable__hello  - set disable__hello until UTC timestamp reached
        timed_exp_raw = query.get('timed_exp', [])
        t_exps = []
        lt = re.compile(r'^lt(?P<timestamp>[0-9]+)$', re.IGNORECASE)
        gt = re.compile(r'^gt(?P<timestamp>[0-9]+)$', re.IGNORECASE)
        now = int(time.time()+time.timezone)
        for exp in timed_exp_raw:
            try:
                t_exps += json.loads(exp)
            except ValueError:
                t_exps += exp.split(',')
        for exp in t_exps:
            parts = [i for i in exp.split("__") if i != ""]
            failed = False
            flag_parts = []
            for part in parts:
                lt_mo = lt.match(part)
                gt_mo = gt.match(part)
                if lt_mo is not None or gt_mo is not None:
                    if lt_mo is not None and not now < int(lt_mo.group('timestamp')):
                        failed = True
                        break
                    elif gt_mo is not None and not now > int(gt_mo.group('timestamp')):
                        failed = True
                        break
                else:
                    flag_parts.append(part)
            if not failed:
                experiments.append("__".join(flag_parts))

        return Experiments(experiments)

    # helpers
    def _check_accept_chat(self, vid):
        accept_rate = float(os.environ.get('CRMBOT_WEBIM_CHAT_ACCEPT_RATE', '1.0'))
        hvid = zlib.crc32(vid) & 0xFFFFFFFF
        return hvid / float(0xFFFFFFFF) <= accept_rate

    def _decline_chat(self, resp):
        logger.info('Declining chat')
        set_json_response(resp, {}, falcon.HTTP_418)

    def _build_req_info(self, req, text='', text_source=Utterance.TEXT_INPUT_SOURCE):
        if text_source == Utterance.SUGGESTED_INPUT_SOURCE:
            event = SuggestedInputEvent(text=text)
        elif text_source == Utterance.TEXT_INPUT_SOURCE:
            event = TextInputEvent(text=text)
        else:  # file
            event = ImageInputEvent()
        req_info = ReqInfo(
            uuid=self._get_chat_id(req),
            client_time=datetime.now(tz=pytz.timezone("Europe/Moscow")),
            app_info=AppInfo(
                app_id=self._get_app_id(),
                app_version=self._get_version(),
                os_version="0.0.0",
                platform="unknown",
                device_manufacturer="unknown",
                device_model="unknown"
            ),
            utterance=Utterance(text=text, input_source=text_source),
            request_id=req.context['reqid'],
            device_id=str(uuid.uuid3(uuid.NAMESPACE_URL, req.uri)),
            event=event,
            experiments=self._get_experiments(req)
        )
        req_info.additional_options['market_reqid'] = req.context['market_reqid']
        return req_info

    # business logic
    def on_post(self, req, resp, app_id):
        self._validate_request_schema(req)

        chat_id = self._get_chat_id(req)
        user_info, req_info = self._parse_request(req)
        connector = self.get_or_create_connected_app(app_id)

        if user_info is not None:
            if not self._check_accept_chat(str(chat_id)):
                self._decline_chat(resp)
                return
            connector.handle_user_info(user_info)

        if req_info is not None:
            result = connector.handle_request(req_info=req_info)
            self._respond_with_result(req, resp, result, app_id)
            resp.context['result'] = result
        else:
            self._respond_ok(resp)


class WebimV1Resource(BaseWebimResource):
    def __init__(self, *args, **kwargs):
        super(WebimV1Resource, self).__init__(*args, **kwargs)

    def _validate_request_schema(self, req):
        try:
            validate_json(req.context['body'], webim_schema_v1)
        except SchemaValidationError as e:
            raise ValidationError(unicode(e))

    def _get_chat_id(self, req):
        return uuid.UUID(req.context['body']['chat']['id'])

    def _get_version(self):
        return "1.0"

    def _get_app_id(self):
        return "webim_crm_bot"

    def _respond_ok(self, resp):
        set_json_response(resp, {}, falcon.HTTP_200)

    def _parse_message(self, req, data):
        if data['kind'] == 'visitor':
            if data['text'].startswith('/service'):
                return None
            return self._build_req_info(req, data['text'], Utterance.TEXT_INPUT_SOURCE)
        elif data['kind'] == 'keyboard_response':
            return self._build_req_info(req, data['response']['button']['text'], Utterance.SUGGESTED_INPUT_SOURCE)
        else:
            raise ValidationError('Unsupported message kind "%r"' % data['kind'])

    def _parse_request(self, req):
        user_info = None
        msg = None
        event_type = req.context['body'].get("event", None)
        if event_type == "new_chat":
            user_info = UserInfo(
                req_info=self._build_req_info(req),
                chat_id=self._get_chat_id(req),
                uri=req.uri,
                req_id=req.context['reqid'],
                session_data={'visitor_id': req.context['body']['visitor']['id'], 'first_request': 1}
            )
            if 'messages' in req.context['body']:
                for message in reversed(req.context['body']['messages']):
                    msg = self._parse_message(req, message)
                    if msg is not None:
                        break
        elif event_type == "new_message":
            msg = self._parse_message(req, req.context['body'])
        else:
            raise ValidationError('Unsupported event type "%r"' % event_type)
        return user_info, msg

    def _respond_with_result(self, req, resp, result, app_id):
        for message in result.messages:
            if self._check_redirect(message):
                set_json_response(resp, {'has_answer': False}, falcon.HTTP_200)
                return

        messages = []
        for message in result.messages:
            mjson = self._message_to_json(message)
            if mjson is not None:
                messages.append(mjson)
        if result.keyboard is not None and not result.keyboard.empty():
            mjson = self._message_to_json(result.keyboard)
            if mjson is not None:
                messages.append(mjson)
        if len(messages) > 0:
            body = {'has_answer': True, 'messages': messages}
        else:
            body = {'has_answer': False}
        set_json_response(resp, body, result.status_code)

    def _check_redirect(self, msg):
        return isinstance(msg, OperatorRedirect) or isinstance(msg, DepartmentRedirect)

    def _message_to_json(self, msg):
        if isinstance(msg, TextMessage) or isinstance(msg, Keyboard):
            return msg.to_json()
        else:
            logger.info('Warning, cannot respond with {}'.format(msg))


class WebimAPIError(Exception):
    pass


class DelayedResponseResource(BaseWebimResource):
    def __init__(self, *args, **kwargs):
        self.response_storage = {}
        super(DelayedResponseResource, self).__init__(*args, **kwargs)

    def _make_auth_headers(self):
        raise NotImplementedError

    def _get_base_url(self):
        raise NotImplementedError

    def _get_chat_id_from_message(self, message):
        raise NotImplementedError

    def _prepare_message(self, response, msg, chat_id):
        raise NotImplementedError

    def _prepare_keyboard(self, response, kbd, chat_id):
        raise NotImplementedError

    def _check_response(self, url, response):
        pass

    def store_response(self, req, response):
        self.response_storage[req.context['market_reqid']] = response

    def _respond_with_result(self, req, resp, result, app_id):
        chat_id = self._get_chat_id(req)
        response = []
        for message in result.messages:
            self._prepare_message(response, message, chat_id)
        if result.keyboard is not None and not result.keyboard.empty():
            self._prepare_keyboard(response, result.keyboard, chat_id)
        if len(response) > 0:
            self.store_response(req, {'response': response, 'app_id': app_id})
        if result.status_code == falcon.HTTP_200:
            self._respond_ok(resp)
        else:
            set_json_response(resp, None, result.status_code)

    def respond_to_user(self, req):
        header = next((i[1] for i in req.headers if i[0] == self._settings.REQUID_HEADER.upper()), None)
        if header is None:
            return
        req_id = header
        response = self.response_storage.pop(req_id, None)
        if response is None:
            return

        url = self._get_base_url()
        logger.info(to_json_str({"Req_id": req_id, "Response": response}))
        for message in response['response']:
            headers = self._make_auth_headers()
            if 'headers' in message:
                headers.update(message['headers'])
            try:
                self._do_post(url=url+message['url'], headers=headers, body=message['body'])
            except WebimAPIError:
                if 'redirect' in message.get('url', ''):
                    self._handle_redirect_error(message, response['app_id'])

    def _do_post(self, url, headers, body):
        if os.environ.get('WEBIM_FORBID_REQUESTS') == '1':
            logger.warning("Outgoing requests are forbidden! Not requesting {}".format(
                to_json_str({"url": url, "headers": headers, "body": body})))
            return

        try:
            resp = requests.post(url=url, json=body, headers=headers, timeout=self._settings.REQUEST_TIMEOUT)
        except Timeout:
            logger.error("Timeout on {}: {}".format(url, body))
            raise WebimAPIError
        logger.info(to_json_str(
            {
                "URL": url, "request": {"body": body, "reqid": headers.get(self._settings.REQUID_HEADER)},
                "response": {"body": resp.json(), "code": resp.status_code}
            }))
        if not resp.ok:
            logger.error(
                "Request to webim returned bad http code. URL: {} Request: {} Response: {}".format(
                    url, to_json_str(body), resp
                ))
            raise WebimAPIError
        self._check_response(url, resp)

    def _handle_redirect_error(self, message, app_id):
        chat_id = self._get_chat_id_from_message(message['body'])
        req_info = ReqInfo(
            uuid=chat_id,
            dialog_id=chat_id,
            client_time=datetime.now(tz=pytz.timezone("Europe/Moscow")),
            app_info=AppInfo(
                app_id=self._get_app_id(),
                app_version=self._get_version(),
                os_version="0.0.0",
                platform="unknown",
                device_manufacturer="unknown",
                device_model="unknown"
            )
        )
        text = self.get_or_create_connected_app(app_id).vins_app.get_redirect_error_text(req_info)

        response = []
        self._prepare_message(response, msg=TextMessage(text=text), chat_id=chat_id)
        msg = response[-1]
        headers = self._make_auth_headers()
        url = self._get_base_url() + msg['url']

        self._do_post(url=url, headers=headers, body=msg['body'])


class WebimV2Resource(DelayedResponseResource):
    def __init__(self, *args, **kwargs):
        self.response_storage = {}
        super(WebimV2Resource, self).__init__(*args, **kwargs)

    def _validate_request_schema(self, req):
        try:
            validate_json(req.context['body'], webim_schema_v2)
        except SchemaValidationError as e:
            raise ValidationError(unicode(e))

    def _get_chat_id(self, req):
        event_type = req.context['body']['event']
        if event_type == 'new_message':
            return uuid.UUID(int=req.context['body']['chat_id'])
        elif event_type == 'new_chat':
            return uuid.UUID(int=req.context['body']['chat']['id'])
        else:
            raise ValidationError('Unsupported event type "%r"' % event_type)

    def _get_app_id(self):
        return "webim_crm_bot"

    def _get_version(self):
        return "2.0"

    def _get_base_url(self):
        return os.environ['WEBIM_URL']

    def _get_chat_id_from_message(self, message):
        return message['chat_id']

    def _respond_ok(self, resp):
        set_json_response(resp, {"result": "ok"}, falcon.HTTP_200)

    def _parse_message(self, req, data):
        if data['kind'] == 'visitor':
            if data['text'].startswith('/service'):
                return None
            return self._build_req_info(req, data['text'], Utterance.TEXT_INPUT_SOURCE)
        elif data['kind'] == 'keyboard_response':
            return self._build_req_info(req, data['data']['button']['text'], Utterance.SUGGESTED_INPUT_SOURCE)
        elif data['kind'] == 'file_visitor':
            return self._build_req_info(req, '', Utterance.IMAGE_INPUT_SOURCE)
        else:
            raise ValidationError('Unsupported message kind "%r"' % data['kind'])

    def _parse_request(self, req):
        if req.get_header(self._settings.REQUID_HEADER, None) is None:
            raise RuntimeError(
                "Webim API v2 cannot work without \"{}\" header".format(self._settings.REQUID_HEADER))

        user_info = None
        msg = None
        event_type = req.context['body'].get("event", None)
        if event_type == "new_chat":
            session_data = {'visitor_id': req.context['body']['visitor']['id'], 'first_request': 1}
            fields = copy.deepcopy(req.context['body']['visitor'].get('fields', None))
            if fields is not None:
                session_data['visitor_fields'] = fields
                if 'sec_data' in fields:
                    decryptor = Decryptor()
                    sec_data = decryptor.decrypt(fields['sec_data'])
                    if sec_data is not None:
                        session_data['sec_data'] = sec_data
                    del fields['sec_data']
            user_info = UserInfo(
                req_info=self._build_req_info(req),
                chat_id=self._get_chat_id(req),
                uri=req.uri,
                req_id=req.context['reqid'],
                session_data=session_data
            )
            # it may have array of latest messages
            if "messages" in req.context['body']:
                for message in reversed(req.context['body']['messages']):
                    msg = self._parse_message(req, message)
                    if msg is not None:
                        break
        elif event_type == "new_message":
            msg = self._parse_message(req, req.context['body']['message'])
        else:
            raise ValidationError('Unsupported event type "%r"' % event_type)
        return user_info, msg

    def _prepare_message(self, response, msg, chat_id):
        if isinstance(msg, TextMessage) or isinstance(msg, FileMessage):
            response.append({'url': '/send_message', 'body': {'chat_id': int(chat_id), 'message': msg.to_json()}})
        elif isinstance(msg, OperatorRedirect) or isinstance(msg, DepartmentRedirect):
            msg.chat_id = int(chat_id)
            response.append({'url': '/redirect_chat', 'body': msg.to_json()})
        elif isinstance(msg, CloseChat):
            msg.chat_id = int(chat_id)
            response.append({'url': '/close_chat', 'body': msg.to_json()})
        else:
            logger.info('Warning, cannot respond with {}'.format(msg))

    def _prepare_keyboard(self, response, msg, chat_id):
        if isinstance(msg, Keyboard):
            response.append({'url': '/send_message', 'body': {'chat_id': int(chat_id), 'message': msg.to_json()}})

    def _make_auth_headers(self):
        token = os.environ['WEBIM_AUTH_TOKEN']
        return {
            'Authorization': 'Token ' + token,
            'Content-Type': 'application/json'
        }

    def _check_response(self, url, response):
        if response.json().get("result", "") != "ok":
            logger.error(
                "Webim returned error code. URL: {} Json Response: {}".format(url, to_json_str(response.json())))
            raise WebimAPIError
