# coding: utf-8
from __future__ import unicode_literals

import logging
from vins_sdk.connectors import ConnectorBase


logger = logging.getLogger(__name__)


class SpeechKitConnector(ConnectorBase):
    def __init__(self, listen_by_default=False, **kwargs):
        super(SpeechKitConnector, self).__init__(**kwargs)
        self._listen_by_default = listen_by_default

    def handle_request(self, req_info, **kwargs):
        response = self._vins_app.handle_request(req_info=req_info)
        if response.should_listen is None:
            response.should_listen = self._listen_by_default
        return response
