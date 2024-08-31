import json
import logging
import urllib.parse

import certifi
import tenacity
from tornado.httpclient import AsyncHTTPClient, HTTPError, HTTPRequest, HTTPResponse
from traitlets import Bool, Integer
from traitlets.config import Configurable

from jupytercloud.backend.lib.util.logging import LoggingContextMixin


class JCHTTPError(HTTPError):
    def __init__(self, code=None, message=None, response=None, exc_repr=None):
        super().__init__(code=code, message=message, response=response)

        self.exc_repr = exc_repr

    @classmethod
    def from_raw_exception(cls, e, exc_repr=None):
        return cls(
            code=e.code,
            message=e.message,
            response=e.response,
            exc_repr=exc_repr,
        )

    def __str__(self):
        result = super().__str__()
        if self.response and self.response.body:
            result += f' ({self.response.body})'
        if self.exc_repr:
            result = f'[{self.exc_repr}] {result}'
        return result


class AsyncHTTPClientMixin(LoggingContextMixin, Configurable):
    _http_client = None

    oauth_token = None

    validate_cert = True
    use_pycurl = Bool(config=True, default_value=True)

    retry_stop_timeout = Integer(120, config=True)
    codes_to_retry = (429, 502, 503, 504, 599)
    messages_to_not_retry = {
        'SSL certificate problem: certificate has expired'
    }
    exceptions_to_retry = (TimeoutError, )
    max_clients = 100

    user_agent = 'JupyterCloud Backend (abc id: 2142)'

    @property
    def log_context(self):
        return None

    @property
    def http_client(self):
        client_defaults = {
            'validate_cert': self.validate_cert,
            'user_agent': self.user_agent,
            'ca_certs': certifi.where(),
        }

        if self._http_client is None:
            try:
                if self.use_pycurl:
                    AsyncHTTPClient.configure('tornado.curl_httpclient.CurlAsyncHTTPClient')
                    self.log.info('🏃 using cURL in Tornado 🏃')
                else:
                    self.log.info('🐢 cURL disabled! slow fallback 🐢')
            except ImportError:
                self.log.info('🐢 no cURL found! slow fallback 🐢')

            self._http_client = AsyncHTTPClient(
                max_clients=self.max_clients,
                defaults=client_defaults,
                client_name=self.__class__.__name__,
            )
        return self._http_client

    def get_headers(self):
        headers = {
            'Content-Type': 'application/json',
        }
        if self.oauth_token:
            headers['Authorization'] = f'OAuth {self.oauth_token}'

        return headers

    @property
    def exc_repr(self):
        return self.__class__.__name__

    _retrier = None

    def retry_condition(self, exception):
        return (
            isinstance(exception, HTTPError) and
            exception.code in self.codes_to_retry and
            getattr(exception, 'message', '') not in self.messages_to_not_retry or
            isinstance(exception, self.exceptions_to_retry)
        )

    @property
    def fetch_with_retry(self):
        if self._retrier is None:
            self._retrier = tenacity.AsyncRetrying(
                wait=tenacity.wait_random_exponential(multiplier=1, max=60),
                stop=tenacity.stop_after_delay(self.retry_stop_timeout),
                retry=tenacity.retry_if_exception(self.retry_condition),
                before_sleep=tenacity.before_sleep_log(self.log, logging.INFO),
                reraise=True,
            ).wraps(self.http_client.fetch)

        return self._retrier

    async def _raw_request(
        self,
        url,
        *,
        data=None,
        params=None,
        method='POST',
        raise_error=True,
        do_retries=True,
        **kwargs,
    ):
        headers = self.get_headers() | kwargs.pop('headers', {})

        # for now it maybe yarl.URL, but maybe str
        url = str(url)

        if data:
            assert not kwargs.get('body')
            kwargs['body'] = json.dumps(data)

        if params:
            assert '?' not in url
            params_str = urllib.parse.urlencode(params, doseq=True)
            url = f'{url}?{params_str}'

        self.log.debug('%s requesting %s with do_retries=%s', method, url, do_retries)

        response = None

        if do_retries:
            fetch = self.fetch_with_retry
        else:
            fetch = self.http_client.fetch

        request = HTTPRequest(
            url,
            headers=headers,
            method=method,
            **kwargs,
        )

        try:
            response = await fetch(request, raise_error=True)
        except HTTPError as e:
            response = e.response

            error = JCHTTPError.from_raw_exception(e, self.exc_repr)
            if response is not None:
                response.error = error

            if raise_error:
                raise error from e
        finally:
            resp = response or HTTPResponse(code=0, request=request)
            result_code = f'have result code {resp.code}'

            self.log.debug(
                '%s request %s %s (%.2f sec)',
                method,
                url,
                result_code,
                resp.request_time or 0,
            )

        if response.error:
            response.error = JCHTTPError.from_raw_exception(response.error, self.exc_repr)

        return response
