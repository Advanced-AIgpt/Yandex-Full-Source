# coding: utf-8

import logging
import re
import attr
import vins_core.dm.response as dm_response

from datetime import datetime
from ylog.context import LogContext

from vins_core.dm.request import ReqInfo, AppInfo, Experiments
from vins_core.dm.request_events import TextInputEvent, SuggestedInputEvent, ServerActionEvent
from vins_core.utils.datetime import parse_tz
from alice.vins.api_helper.resources import (
    parse_json_request,
    set_json_response,
    dump_request_for_log,
    dump_response_for_log,
)
from vins_api.common.resources import BaseConnectedAppResource
from vins_api.external_skill.connector import ExternalSkillConnector
from vins_api.external_skill.schemas import external_skill_schema

logger = logging.getLogger(__name__)


def parse_ua(useragent):
    match = re.match(
        r'^(?P<app_id>.*)/(?P<app_version>.*) \((?P<device>.*); (?P<platform>.*?) (?P<os_version>.*?)\)$',
        useragent
    )
    if match:
        return match.groupdict()
    else:
        return {'app_id': useragent}


@attr.s
class Button(object):
    title = attr.ib()
    url = attr.ib(default=None)
    hide = attr.ib(default=True)
    payload = attr.ib(default=None)

    def to_dict(self):
        res = {'title': self.title, 'hide': self.hide}

        if self.payload is not None:
            res['payload'] = self.payload

        if self.url is not None:
            res['url'] = self.url

        return res


class ExternalSkillResource(BaseConnectedAppResource):
    connector_cls = ExternalSkillConnector

    def get_req_info(self, data):
        session = data['session']
        meta = data['meta']
        app_info = parse_ua(meta['client_id'])
        request = data['request']

        request_text = request['command'] or request['original_utterance']
        request_payload = request.get('payload', {})

        if request['type'] == 'SimpleUtterance':
            event = TextInputEvent(request_text, payload=request_payload)
        elif request['type'] == 'ButtonPressed':
            if (
                    request_payload and
                    'callback_name' in request_payload and
                    'callback_args' in request_payload
            ):
                event = ServerActionEvent(
                    request_payload['callback_name'],
                    payload=request_payload['callback_args'],
                )
            else:
                event = SuggestedInputEvent(request_text, payload=request_payload)
        else:
            raise ValueError('Unknown request type %s' % request['type'])

        return ReqInfo(
            uuid=session['user_id'],
            client_time=datetime.now(tz=parse_tz(meta['timezone'])),
            app_info=AppInfo(
                app_id=app_info.get('app_id'),
                app_version=app_info.get('app_version'),
                os_version=app_info.get('os_version'),
                platform=app_info.get('platform'),
            ),
            utterance=event.utterance,
            lang=meta['locale'],
            experiments=Experiments(meta.get('experiments', {})),
            reset_session=session['new'],
            additional_options={
                'skill_id': session.get('skill_id', ''),
                'session_id': session['session_id'],
                'original_utterance': request['original_utterance'],
            },
            request_id=str(session['message_id']),
            event=event,
        )

    def on_post(self, req, resp, app_id, **kwargs):
        data = parse_json_request(req, external_skill_schema)

        req_info = self.get_req_info(data)
        with LogContext(request_id=str(req_info.request_id),
                        uuid=str(req_info.uuid),
                        skill_id=req_info.additional_options['skill_id']):
            logger.debug('Received request: %s', dump_request_for_log(data))
            app = self.get_or_create_connected_app(app_id)
            result = app.handle_request(req_info)

            serialized = self.serialize_response(result, req_info)
            set_json_response(resp, serialized)
            logger.debug('External skill response %s', dump_response_for_log(resp))

    def _get_card_text(self, result):
        text = []
        for card in result.cards:
            if isinstance(card, (dm_response.SimpleCard, dm_response.CardWithButtons)):
                text.append(card.text)

        return '\n'.join(text)

    def _make_button(self, btn):
        for directive in btn.directives:
            if (
                    directive.type == 'client_action' and
                    directive.name == 'open_uri'
            ):
                return Button(title=btn.title, hide=False, url=directive.payload['uri'])
            elif directive.type == 'server_action':
                payload = {
                    'callback_name': directive.name,
                    'callback_args': directive.payload,
                }
                return Button(title=btn.title, hide=True, payload=payload)
        return Button(title=btn.title, hide=True)

    def _get_buttons(self, result):
        buttons = [self._make_button(btn) for btn in result.suggests]

        for card in result.cards:
            if card.type == 'text_with_button':
                for btn in card.buttons:
                    buttons.append(self._make_button(btn))

        return buttons

    def serialize_response(self, result, req_info):
        text = self._get_card_text(result)
        buttons = self._get_buttons(result)

        has_errors = any(map(lambda x: isinstance(x, dm_response.ErrorMeta), result.meta))

        return {
            'version': '1.0',
            'session': {
                'session_id': req_info.additional_options['session_id'],
                'message_id': int(req_info.request_id),
                'user_id': str(req_info.uuid),
            },
            'response': {
                'text': text,
                'tts': result.voice_text,
                'buttons': [b.to_dict() for b in buttons],
                'end_session': has_errors,
            },
        }
