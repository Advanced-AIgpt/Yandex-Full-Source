# coding: utf-8

import logging
import os
# import pkg_resources
import requests
import json
from urlparse import urlparse

from requests.adapters import HTTPAdapter
from vins_core.utils.config import get_setting
from vins_core.utils.strings import smart_unicode
from vins_core.utils.metrics import sensors
from rtlog.thread_local import log_child_activation_started, log_child_activation_finished

from library.python import svn_version

# version = pkg_resources.get_distribution('vins_core').version
version = str(svn_version.svn_revision())
logger = logging.getLogger(__name__)


class CertificationAuthority(object):
    _initialized = False
    _cert = None

    @classmethod
    def get_cert(cls):
        if not cls._initialized:
            cls._initialized = True
            standard_ca_bundle_path = '/etc/ssl/certs/ca-certificates.crt'
            ca_bundle_path = get_setting('CERT_PATH', standard_ca_bundle_path)
            if os.path.exists(ca_bundle_path):
                cls._cert = ca_bundle_path
        return cls._cert

    @classmethod
    def cert_exist(cls):
        return cls.get_cert() is not None


class BaseHTTPAPI(object):
    MAX_RETRIES = 0
    HEADERS = {
        'User-Agent': 'yandex-vins/' + version,
    }
    CONNECTION_TIMEOUT = 0.5  # seconds
    TIMEOUT = 1.0  # seconds
    PROXY_URL = get_setting('PROXY_URL', default='')
    PROXY_SKIP = get_setting('PROXY_SKIP', default='')
    PROXY_DEFAULT_HEADERS = get_setting('PROXY_DEFAULT_HEADERS', default='{}')

    def __init__(self, timeout=None, connection_timeout=None, max_retries=None, headers=None, **kwargs):
        self._timeout = timeout or self.TIMEOUT
        self._connection_timeout = connection_timeout or self.CONNECTION_TIMEOUT
        self._headers = headers or {}
        self._max_retries = max_retries or self.MAX_RETRIES
        self._sessions = {self._session_key(): self.create_session()}
        self._default_headers = json.loads(self.PROXY_DEFAULT_HEADERS)

    def create_session(self):
        session = requests.Session()
        if CertificationAuthority.cert_exist():
            session.verify = CertificationAuthority.get_cert()
        session.headers.update(self.HEADERS)
        session.headers.update(self._headers)
        session.mount('http://', HTTPAdapter(max_retries=self._max_retries))
        session.mount('https://', HTTPAdapter(max_retries=self._max_retries))
        return session

    def _session_key(self):
        return os.getpid()

    @property
    def http(self):
        key = self._session_key()
        if key in self._sessions:
            return self._sessions[key]
        else:
            session = self.create_session()
            self._sessions[key] = session
            return session

    def _prepare_kwargs(self, kwargs):
        kwargs['timeout'] = self._connection_timeout, self._timeout
        request_id = kwargs.get('request_id')
        if request_id is not None:
            request_id_headers = {'X-Request-Id': request_id}
            if 'headers' not in kwargs:
                kwargs['headers'] = request_id_headers
            else:
                kwargs['headers'].update(request_id_headers)
        if 'request_id' in kwargs:
            kwargs.pop('request_id')

    def _postprocess_response(self, response):
        pass

    def _log_response(self, response):
        data = {
            'request_method': response.request.method,
            'request_url': response.request.url,
            'response_status_code': response.status_code,
            'response_time': response.elapsed.total_seconds(),
        }
        logger.info('%s request to %s returned %s and took %s seconds',
                    data['request_method'],
                    data['request_url'],
                    data['response_status_code'],
                    data['response_time'],
                    extra={'data': data},
                    )
        url = urlparse(response.url)
        if url.netloc.startswith('['):
            label_netloc = 'IPv6:' + url.netloc
        else:
            label_netloc = url.netloc
        labels = {
            'netloc': label_netloc,
            'status_code': str(response.status_code),
        }
        sensors.inc_counter('source_http_request', labels=labels)
        sensors.set_sensor(
            'source_http_request_time',
            response.elapsed.total_seconds() * 1000,  # milliseconds
            labels=labels,
        )

    def _log_error(self, e):
        data = {
            'request_method': e.request and e.request.method,
            'request_url': e.request and e.request.url,
            'request_exception_message': smart_unicode(e.message),
        }
        logger.warning(
            '%s request to %s raised error: %s',
            data['request_method'],
            data['request_url'],
            data['request_exception_message'],
            extra={'data': data},
        )

    def _request(self, method, url, *args, **kwargs):
        self._prepare_kwargs(kwargs)

        headers = kwargs.get('headers', None)
        if headers is None:
            headers = {}
            kwargs['headers'] = headers

        rtlog_token = None
        request_label = kwargs.pop('request_label', None)
        if request_label:
            rtlog_token = log_child_activation_started(request_label)
            if rtlog_token:
                headers['X-RTLog-Token'] = rtlog_token

        # setup proxy if needed
        if self.PROXY_URL and url:
            # check filters
            if not self.PROXY_SKIP or not any(url.startswith(skip_url) for skip_url in self.PROXY_SKIP.split(';')):
                # set path to proxy server
                headers['X-Host'] = url
                url = self.PROXY_URL

                # set default headers if missing
                # HTTP message header keys are case-insensitive by standard, so check them by lowercasing
                header_keys = [h.lower() for h in headers]
                for header, value in self._default_headers.items():
                    if header.lower() not in header_keys:
                        headers[header] = value

        try:
            response = self.http.request(method, url, *args, **kwargs)
            if rtlog_token:
                log_child_activation_finished(rtlog_token, True)
            self._log_response(response)
        except requests.exceptions.RequestException as e:
            if rtlog_token:
                log_child_activation_finished(rtlog_token, False)
            self._log_error(e)
            raise
        self._postprocess_response(response)
        return response

    def get(self, url, *args, **kwargs):
        return self._request('GET', url, *args, **kwargs)

    def post(self, url, *args, **kwargs):
        return self._request('POST', url, *args, **kwargs)
