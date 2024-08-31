# coding: utf-8
from __future__ import unicode_literals

import logging
import os
import re

from vins_sdk.connectors import ConnectorBase
from vins_core.dm.response import SimpleCard, ActionButton, ServerActionDirective
from vins_api.webim.primitives import TextMessage, Button, Keyboard, OperatorRedirect, \
    DepartmentRedirect, CloseChat, VinsResponse
from vins_api.speechkit.resources.common import http_status_code_for_response
from vins_core.utils.data import to_json_str

logger = logging.getLogger(__name__)


class WebimConnector(ConnectorBase):
    class RedirectMatcher(object):
        def __init__(self, pattern, redirect_class, args_map):
            self.pattern = re.compile(pattern)
            self.redirect_class = redirect_class
            self.args_map = args_map

        def make_redirect(self, mo):
            init_args = {
                k: v.get('value') if 'value' in v else mo.group(v.get('group'))
                for k, v in self.args_map.iteritems()
            }
            return self.redirect_class(**init_args)

        def purge_redirect(self, message, mo):
            return message.replace(mo.group(0), '').strip()

        def process_message(self, message):
            mo = self.pattern.search(message)
            if mo:
                redirect = self.make_redirect(mo)
                new_message = self.purge_redirect(message, mo)
                return redirect, new_message
            return None, message

    def __init__(self, *args, **kwargs):
        super(WebimConnector, self).__init__(*args, **kwargs)
        self.redirect_matchers = [
            WebimConnector.RedirectMatcher(
                pattern=r'OPERATOR_REDIRECT_(?P<operator_id>[0-9]*)',
                redirect_class=OperatorRedirect,
                args_map={'operator_id': {'group': 'operator_id'}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'OPERATOR_REDIRECT',
                redirect_class=DepartmentRedirect,
                args_map={'dep_key': {'value': os.environ["WEBIM_DEFAULT_DEPARTMENT_KEY"]}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'DEPARTMENT_REDIRECT_(?P<dep_key>[a-zA-Zа-яА-Я0-9.-]*)',
                redirect_class=DepartmentRedirect,
                args_map={'dep_key': {'group': 'dep_key'}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'CLOSE_CHAT',
                redirect_class=CloseChat,
                args_map={}
            )
        ]

    def handle_request(self, req_info, **kwargs):
        response = self._vins_app.handle_request(req_info=req_info)
        resp = self.parse_vins_response(response)
        resp.status_code = http_status_code_for_response(response)
        return resp

    def handle_user_info(self, user_info):
        session = self.vins_app.load_or_create_session(user_info.req_info)
        for k, v in user_info.session_data.iteritems():
            session.set(k, v)
        logger.info("Writing session data for uuid: {} data: {}".format(
            user_info.req_info.uuid, to_json_str(user_info.session_data)
        ))
        self.vins_app.save_session(session, user_info.req_info)

    def parse_vins_response(self, response):
        messages = []
        redirect = None
        intent_name = None
        # intent name
        try:
            intent_name = response.features['form_info'].intent
        except (KeyError, TypeError, AttributeError):
            pass
        # messages
        for card in response.cards:
            if isinstance(card, SimpleCard):
                text = card.text
                for matcher in self.redirect_matchers:
                    r, text = matcher.process_message(text)
                    if r is not None:
                        redirect = r
                        break
                if len(text) > 0:
                    messages.append(TextMessage(text=text))
        # suggests
        keyboard = Keyboard()
        for suggest in response.suggests:
            if isinstance(suggest, ActionButton):
                same_row = False
                for directive in suggest.directives:
                    if isinstance(directive, ServerActionDirective):
                        try:
                            same_row = directive.payload['suggest_block']['data']['nobr']
                        except (KeyError, TypeError):
                            same_row = False
                        break
                keyboard.add_button(button=Button(text=suggest.title), continue_row=same_row)
        if redirect is not None:
            messages.append(redirect)
        if not keyboard.empty():
            return VinsResponse(messages=messages, intent=intent_name, keyboard=keyboard)
        else:
            return VinsResponse(messages=messages, intent=intent_name)
