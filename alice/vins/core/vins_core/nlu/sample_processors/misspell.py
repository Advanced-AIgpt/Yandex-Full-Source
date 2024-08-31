# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging
import re

from requests import RequestException

from vins_core.common.sample import Sample
from vins_core.nlu.sample_processors.base import BaseSampleProcessor
from vins_core.ext.base import BaseHTTPAPI
from vins_core.common.utterance import Utterance
from vins_core.utils.config import get_setting
from vins_core.utils.metrics import sensors
from vins_core.utils.strings import smart_utf8

logger = logging.getLogger(__name__)


class MisspellSamplesProcessor(BaseSampleProcessor):
    """Sample processor for fixing grammar errors and misspells in an input utterance.
    """

    CONFIDENT_MISSPELL_CODE = 10000
    URL = get_setting('MISSPELL_URL', yenv={
        'development': 'http://misc-spell-dev.yandex.net:19036/misspell.json/check',
        'testing': 'http://misc-spell-dev.yandex.net:19036/misspell.json/check',
        'production': 'http://misc-spell.yandex.net:19036/misspell.json/check'
    })

    REGEXP = re.compile(r'(\xad\(|\)\xad)')  # '\xad' is soft hyphen symbol
    MISSPELL_TIMEOUT = float(get_setting('MISSPELL_TIMEOUT', 0.2))
    MISSPELL_SRV = get_setting('MISSPELL_SRV', 'vins')

    def __init__(self, **kwargs):
        super(MisspellSamplesProcessor, self).__init__(**kwargs)

        self._http_session = BaseHTTPAPI(
            timeout=self.MISSPELL_TIMEOUT,
        )

    @property
    def is_normalizing(self):
        return True

    @sensors.with_timer('misspell_response_time')
    def _request(self, text):
        return self._http_session.get(
            self.URL,
            params={'text': text, 'srv': self.MISSPELL_SRV},
            request_label=b'misspell:{0}'.format(smart_utf8(text))
        )

    def _fix_misspells(self, text):
        r = self._request(text)
        sensors.inc_counter('misspell_response', labels={'status_code': r.status_code})
        r.raise_for_status()

        r = r.json()
        repaired_text = r.get('text')
        if repaired_text and r.get('r') == self.CONFIDENT_MISSPELL_CODE:
            return self.REGEXP.sub('', repaired_text)
        else:
            return text

    def _process(self, sample, session, *args, **kwargs):
        # If utterance is from user text input, then there is need to run misspeller.
        if sample.utterance.input_source == Utterance.TEXT_INPUT_SOURCE:
            try:
                fixed_text = self._fix_misspells(sample.text)
                fixed_text_tokens = fixed_text.split()

                # If sample has non-trivial tags and amounts of tokens are different, do not apply misspeller.
                if all(tag == 'O' for tag in sample.tags):
                    return Sample(
                        utterance=sample.utterance, tokens=fixed_text_tokens,
                        app_id=sample.app_id, partially_normalized_text=fixed_text, annotations=sample.annotations
                    )
                elif len(fixed_text_tokens) == len(sample.tokens):
                    return Sample(
                        utterance=sample.utterance, tokens=fixed_text_tokens, tags=sample.tags,
                        app_id=sample.app_id, partially_normalized_text=fixed_text, annotations=sample.annotations
                    )
            except RequestException as e:
                logger.warning('Fixing misspells failed on "{}"\n'
                               'Reason: {}'.format(sample.text, e.message))

        return sample
