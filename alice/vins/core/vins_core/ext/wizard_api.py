# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json
import logging
from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting
from vins_core.utils.metrics import sensors

logger = logging.getLogger(__name__)


class WizardHTTPAPI(BaseHTTPAPI):
    MAX_RETRIES = 3
    TIMEOUT = float(get_setting('WIZARD_TIMEOUT', 0.2))
    WIZARD_URL = get_setting('WIZARD_URL', yenv={
        'development': 'http://hamzard.yandex.net:8891/wizard',
        'testing': 'http://hamzard.yandex.net:8891/wizard',
        'production': 'http://reqwizard.yandex.net:8891/wizard'
    })

    def __init__(self, url=None, **kwargs):
        super(WizardHTTPAPI, self).__init__(**kwargs)
        self._url = url or self.WIZARD_URL

    @sensors.with_timer('wizard_response_time')
    def get_response(self, text, extra_params=None, request_id=None):
        params = {
            b'text': text.encode('utf-8'),
            b'format': b'json',
            b'wizclient': b'megamind',
        }
        if extra_params:
            params.update(extra_params)
        r = self.get(self._url, params=params, request_id=request_id,
                     request_label=b'wizard:{0}'.format(text.encode('utf-8')))
        sensors.inc_counter('wizard_response', labels={'status_code': r.status_code})

        r.raise_for_status()
        return json.loads(r.content, encoding='utf-8')


wizard_http_api = WizardHTTPAPI()
