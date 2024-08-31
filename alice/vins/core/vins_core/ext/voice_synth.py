# coding: utf-8
from __future__ import absolute_import
from __future__ import unicode_literals

import logging
import urllib

from requests.exceptions import RequestException
from vins_core.utils.strings import smart_utf8

from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting

TTS_API_KEY = '308c532f-0a05-4e24-91a5-6e7c99bbb089'
TTS_API_URL = get_setting('TTS_API_URL', default='https://tts.voicetech.yandex.net/')

logger = logging.getLogger(__name__)


class VoiceSynthAPIError(Exception):
    pass


class VoiceSynthAPI(BaseHTTPAPI):
    TIMEOUT = 5

    def __init__(self, url=TTS_API_URL, api_key=TTS_API_KEY, **kwargs):
        super(VoiceSynthAPI, self).__init__(**kwargs)
        self._url = url
        self._api_key = api_key

    def _get(self, params, **kwargs):
        try:
            return self.get(
                self._url + 'generate?' + urllib.urlencode(params),
                request_label=b'tts:{0}'.format(params.get('text')),
                **kwargs
            )
        except RequestException as e:
            logger.error('Speech api communication error: %s', e)
            raise VoiceSynthAPIError(e)

    def submit(self, text, fmt='mp3', speaker='oksana', emotion='good'):
        request = {
            'format': fmt,
            'lang': 'ru-RU',
            'speaker': speaker,
            'emotion': emotion,
            'key': self._api_key,
            'text': smart_utf8(text)
        }

        response = self._get(request)

        return response

    @property
    def url(self):
        return self._url
