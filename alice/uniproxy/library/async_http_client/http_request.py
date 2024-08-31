import copy
import json
import time

from urllib.parse import urlsplit, urlunsplit

from alice.uniproxy.library.async_http_client.settings import HTTP_CLIENT_DEFAULT_REQUEST_TIMEOUT
from alice.uniproxy.library.utils.security import hide_cgi_secrets


# ====================================================================================================================
class HTTPRequest:
    def __init__(self, query, method='GET', headers=None, body=None, request_timeout=None, host=None, retries=1):
        self.host = host
        u = urlsplit(query)
        self.query = urlunsplit(('', '', u.path, u.query, u.fragment)) or '/'

        if not self.host:
            self.host = u.hostname
        self.method = method
        self.code = 0
        self.headers = headers if headers is not None else {}
        self.body = body
        self.request_timeout = request_timeout if request_timeout else HTTP_CLIENT_DEFAULT_REQUEST_TIMEOUT
        self.created_ts = time.time()
        self.queued_ts = self.created_ts
        self.sending_ts = self.created_ts
        self.sent_ts = self.created_ts
        self.recv_ts = self.created_ts
        self.finished_ts = self.created_ts
        self._retries = retries
        self._data = None

    # ----------------------------------------------------------------------------------------------------------------
    def __repr__(self):
        return 'HTTPRequest(%s %s queued=%.6f, sending=%.6f, receiving=%.6f, total=%.6f)' % (
            self.method, self.query,
            self.sending_ts - self.queued_ts,
            self.sent_ts - self.sending_ts,
            self.recv_ts - self.sent_ts,
            self.finished_ts - self.created_ts
        )

    # ----------------------------------------------------------------------------------------------------------------
    def set_retries(self, retries):
        self._retries = retries

    # ----------------------------------------------------------------------------------------------------------------
    def prepare(self, host=None):
        if 'Host' not in self.headers:
            if host:
                self.headers['Host'] = host
            elif self.host:
                self.headers['Host'] = self.host

        req = b''
        req += '{} {} HTTP/1.1\r\n'.format(self.method, self.query).encode('utf-8')

        for key, value in self.headers.items():
            if key.lower() != 'content-length':
                req += '{}: {}\r\n'.format(key, value).encode('utf-8')
            elif key.lower() == 'connection':
                pass

        req += b'Connection: Keep-Alive\r\n'

        if self.body:
            if self.method == 'POST' or self.method == 'DELETE' or self.method == 'PUT':
                req += 'Content-Length: {}\r\n'.format(len(self.body)).encode('utf-8')
                req += b'\r\n'
                req += self.body
        else:
            req += b'\r\n'

        self._data = req
        return self._data

    # ----------------------------------------------------------------------------------------------------------------
    def set_queued(self):
        self.queued_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_sending(self):
        self.sending_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_sent(self):
        self.sent_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_received(self):
        self.recv_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_ready(self):
        self.finished_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_timedout(self):
        self.finished_ts = time.time()

    # ----------------------------------------------------------------------------------------------------------------
    def set_failed(self):
        self.finished_ts = time.time()

    def make_logable(self):
        result = copy.copy(self)
        result.query = hide_cgi_secrets(result.query)
        return result


# ====================================================================================================================
class HTTPResponse(object):
    def __init__(self, code, body, request=None):
        super(HTTPResponse, self).__init__()
        self.body = body
        self.headers = {}
        self.code = code
        self.request = request
        self.chunked = False
        self.length = 0
        self.gzipped = False
        self.time_info = {}

    # ----------------------------------------------------------------------------------------------------------------
    def __repr__(self):
        return 'HTTPResponse(status=%d, time=%.6f queued=%.6f)' % (
            self.code,
            self.response_time(),
            self.queue_time()
        )

    def serialize(self):
        return (
            f"{self.code}\n" +
            "".join(f"{n}: {v}\n" for n, v in self.headers.items()) +
            "\n" +
            f"{self.body}"
        )

    # ----------------------------------------------------------------------------------------------------------------
    def add_header(self, name, value):
        self.headers[name] = value

    # ----------------------------------------------------------------------------------------------------------------
    def header(self, name, default=None):
        return self.headers.get(name.lower(), default)

    # ----------------------------------------------------------------------------------------------------------------
    def text(self, encoding='utf-8'):
        if self.body:
            return self.body.decode(encoding)
        else:
            return ''

    # ----------------------------------------------------------------------------------------------------------------
    def json(self, encoding='utf-8'):
        text = self.text(encoding)
        return json.loads(text)

    # ----------------------------------------------------------------------------------------------------------------
    def response_time(self):
        if not self.request:
            return 0.0
        return self.request.recv_ts - self.request.sending_ts

    # ----------------------------------------------------------------------------------------------------------------
    def queue_time(self):
        if not self.request:
            return 0.0
        return self.request.sending_ts - self.request.queued_ts

    # ----------------------------------------------------------------------------------------------------------------
    def processing_time(self):
        if not self.request:
            return 0.0
        return self.request.finished_ts - self.request.queued_ts


# ====================================================================================================================
class HTTPError(Exception):
    CODE_REQUEST_TIMEOUT = 599
    CODE_CONNECT_TIMEOUT = 598
    CODE_CONNECTION_DROPPED = 597
    CODE_CONNECTION_CANCELED = 596

    @classmethod
    def is_internal(cls, code):
        return cls.CODE_CONNECTION_DROPPED <= code <= cls.CODE_REQUEST_TIMEOUT

    def __init__(self, code=None, body=None, request=None):
        super(HTTPError, self).__init__()
        self.code = code
        self.body = body
        self.request = request

    # ----------------------------------------------------------------------------------------------------------------
    def text(self):
        if self.body is None:
            return None
        return self.body.decode('utf-8')

    # ----------------------------------------------------------------------------------------------------------------
    def json(self):
        return json.loads(self.text())

    # ----------------------------------------------------------------------------------------------------------------
    def __str__(self):
        return 'HTTPError(code=%d, body=%s)' % (
            self.code,
            '' if self.body is None else self.body.decode('utf-8', errors='replace')
        )

    # ----------------------------------------------------------------------------------------------------------------
    def __repr__(self):
        return str(self)
