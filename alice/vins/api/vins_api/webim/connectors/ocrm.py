# coding: utf-8
from __future__ import unicode_literals

import logging
import os

from vins_api.webim.primitives import OCRMDepartmentRedirect
from vins_api.webim.connectors.webim import WebimConnector
from vins_core.utils.data import to_json_str

logger = logging.getLogger(__name__)


class OCRMConnector(WebimConnector):
    def __init__(self, *args, **kwargs):
        super(WebimConnector, self).__init__(*args, **kwargs)
        self.redirect_matchers = [
            WebimConnector.RedirectMatcher(
                pattern=r'OPERATOR_REDIRECT_(?P<operator_id>[0-9]*)',
                redirect_class=OCRMDepartmentRedirect,
                args_map={'category': {'group': 'operator_id'}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'OPERATOR_REDIRECT',
                redirect_class=OCRMDepartmentRedirect,
                args_map={'category': {'value': os.environ["WEBIM_DEFAULT_DEPARTMENT_KEY"]}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'DEPARTMENT_REDIRECT_(?P<dep_key>[a-zA-Zа-яА-Я0-9.-]*)',
                redirect_class=OCRMDepartmentRedirect,
                args_map={'category': {'group': 'dep_key'}}
            ),
            WebimConnector.RedirectMatcher(
                pattern=r'CLOSE_CHAT',
                redirect_class=OCRMDepartmentRedirect,
                args_map={}
            )
        ]

    def handle_user_info(self, user_info):
        session = self.vins_app.load_or_create_session(user_info.req_info)
        logger.info("Session: {}".format(session))
        for k, v in user_info.session_data.iteritems():
            session.set(k, v)
        logger.info("Writing session data for uuid: {} data: {}".format(
            user_info.req_info.uuid, to_json_str(user_info.session_data)
        ))
        if not session.has('first_request'):
            session.set('first_request', 1)
            logger.info("SETTING FIRST_REQUEST = 1")
        else:
            session.set('first_request', 0)
            logger.info("SETTING FIRST_REQUEST = 0")
        self.vins_app.save_session(session, user_info.req_info)
