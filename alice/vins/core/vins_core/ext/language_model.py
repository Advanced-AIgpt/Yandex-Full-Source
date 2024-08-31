# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import json
import logging
from vins_core.ext.base import BaseHTTPAPI

logger = logging.getLogger(__name__)


class LanguageModelAPI(BaseHTTPAPI):
    MAX_RETRIES = 3
    TIMEOUT = 0.2
    ERRATUM_3_GRAMM_URL = 'http://erratum-test:19036/lms/getP'
    ERRATUM_5_GRAMM_URL = 'https://test.translate.yandex.net:19443/lms/getP'

    MODE_3_GRAMM = 0
    MODE_5_GRAMM = 1

    def __init__(self, mode=MODE_3_GRAMM, **kwargs):
        super(LanguageModelAPI, self).__init__(**kwargs)
        self._url = self.ERRATUM_3_GRAMM_URL if mode == self.MODE_3_GRAMM else self.ERRATUM_5_GRAMM_URL
        self._mode = mode

    def get_score(self, text):
        params = {
            b'text': text.encode('utf-8'),
            b'lang': b'en-ru',
            b'srv': b'mt-dev',
        }
        r = self.get(self._url, params=params, request_label=b'lang:{0}'.format(text.encode('utf-8')))
        r.raise_for_status()
        result = json.loads(r.content, encoding='utf-8')
        return result if self._mode == self.MODE_5_GRAMM else result["LM"]["FullProb"]
