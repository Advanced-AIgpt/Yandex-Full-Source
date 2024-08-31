# coding: utf-8

import os
import json
import logging

from vins_core.utils.config import get_setting
from vins_core.utils.updater import Updater
from vins_core.utils.data import make_safe_filename, vins_temp_dir

from .base import BaseHTTPAPI

logger = logging.getLogger(__name__)


class YTHttp(BaseHTTPAPI):
    TIMEOUT = 4.0
    HEADERS = {
        'X-YT-Output-Format': json.dumps({'$attributes': {'encode_utf8': False}, '$value': 'json'}),
        'Authorization': 'OAuth %s' % get_setting('YT_TOKEN', prefix='', default=''),
    }


class YtUpdater(Updater):
    def __init__(self, yt_path, file_path=None, **kwargs):
        self._yt_path = yt_path
        self._yt_proxy = get_setting('YT_PROXY', prefix='', default='hahn.yt.yandex.net')
        self._attrs_url = 'http://%s/api/v3/get' % self._yt_proxy
        self._read_table_url = 'http://%s/api/v3/read_table' % self._yt_proxy

        self._http = YTHttp()
        self._last_update = None
        if not file_path:
            file_path = os.path.join(vins_temp_dir(), 'yt_updater__%s' % make_safe_filename(self._yt_path))
        super(YtUpdater, self).__init__(file_path=file_path, **kwargs)

    def _get_resp(self, url, params, stream=False):
        resp = self._http.get(url, params=params, stream=stream, request_label='yt')

        if not resp.ok:
            logger.error('Unexpected response from yt %s %s', resp.status_code, resp.url)
            return None

        return resp

    def _download_content(self, file_obj):
        resp = self._get_resp(
            self._read_table_url, params={'path': self._yt_path}, stream=True,
        )
        if not resp:
            return

        for data in resp.iter_content(1024):
            file_obj.write(data)
        return True

    def _download_to_file(self, file_obj):
        resp = self._get_resp(
            self._attrs_url, params={'path': self._yt_path + '/@modification_time'}
        )

        if not resp:
            return False

        modification_time = resp.json()

        if self._last_update != modification_time:
            self._last_update = modification_time
            return self._download_content(file_obj)
        return False
