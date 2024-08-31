# coding: utf-8
import json
import hashlib
import logging
import requests
from requests.packages.urllib3.util.retry import Retry

from vins_core.ext.base import BaseHTTPAPI
from vins_core.utils.config import get_setting

logger = logging.getLogger(__name__)


class SandboxApi(BaseHTTPAPI):
    MAX_RETRIES = 3
    TIMEOUT = 10
    url = 'https://sandbox.yandex-team.ru/api/v1.0/'
    proxy_url = 'https://proxy.sandbox.yandex-team.ru/'

    def __init__(self, token=None, *args, **kwargs):
        self._token = token or get_setting('SANDBOX_OAUTH_TOKEN', default='')
        super(SandboxApi, self).__init__(*args, **kwargs)

    def create_session(self):
        session = super(SandboxApi, self).create_session()

        if self._token != '':
            session.headers['Authorization'] = 'OAuth ' + self._token

        # add retries to proxy url
        retry = Retry(
            backoff_factor=0.5,
            status_forcelist=[500, 502],
            connect=5,
            read=5,
        )
        session.mount(
            self.proxy_url,
            requests.adapters.HTTPAdapter(max_retries=retry),
        )
        session.mount(
            self.url,
            requests.adapters.HTTPAdapter(max_retries=retry),
        )
        return session

    def get_resource_by_id(self, id):
        res = self.get(self.url + 'resource/' + str(id), request_label='sb:{0}'.format(id))
        res.raise_for_status()
        data = res.json()
        return data

    def get_resources(self, limit=10, query=None):
        expected = {
            'limit', 'order', 'offset', 'task_id', 'type', 'expires',
            'dependant', 'id', 'any_attr', 'state', 'client', 'attrs',
            'owner', 'accessed', 'arch',
        }
        if query is None:
            query = {}

        assert set(query).issubset(expected)

        query = query.copy()
        query['limit'] = limit
        if 'attrs' in query:
            query['attrs'] = json.dumps(query['attrs'])

        res = self.get(self.url + 'resource', params=query, request_label='sb')
        res.raise_for_status()
        return res.json()

    def download_resource(self, id, save_to, check_md5=True):
        url = self.proxy_url + str(id)
        md5 = None

        if check_md5:
            try:
                resource_meta = self.get_resource_by_id(id)
            except requests.RequestException:
                logger.exception("Can't get resource %s info from api", id)
            else:
                if resource_meta['state'] != 'READY':
                    raise ValueError('Resource %s is not in READY state' % id)

                md5 = resource_meta['md5']

        res = self.get(url, stream=True, request_label='sb_proxy:{0}'.format(id))
        res.raise_for_status()

        ref_content_length = int(res.headers['content-length'])

        actual_content_length = 0
        hasher = hashlib.md5()
        for chunk in res.iter_content(chunk_size=4 * 1024):
            if chunk:
                actual_content_length += len(chunk)
                save_to.write(chunk)
                hasher.update(chunk)

        if ref_content_length != actual_content_length:
            raise RuntimeError("Can't retrieve file from url %s. Content lengths mismatch."
                               "Expected %d bytes, got %d." % (url, ref_content_length, actual_content_length))

        if md5 and md5 != hasher.hexdigest():
            raise RuntimeError(
                "Checksum mismatch for resource %s, expected %s, get %s."
                % (url, md5, hasher.hexdigest())
            )
