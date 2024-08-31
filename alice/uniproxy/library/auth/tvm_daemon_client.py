import aiohttp
import json

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPResponse


"""
Here we are tweaking http client which communicates with tvm daemon.
We need NewStyleHttpClientForTvmDaemon in asyncio context where tornado-4 coroutines don't work.
"""


class HttpClientForTvmDaemonBase:
    def __init__(self, port, token):
        self.host = 'localhost'
        self.port = port
        self.token = token

    async def fetch(self, path, headers={}):
        url = f'http://{self.host}:{self.port}{path}'
        headers.setdefault('Authorization', self.token)
        headers.setdefault('Connection', 'Keep-Alive')
        return await self._do_fetch(url, headers)

    async def _do_fetch(self, url, headers={}):
        raise NotImplementedError()


class OldStyleHttpClientForTvmDaemon(HttpClientForTvmDaemonBase):
    def __init__(self, port, token, **http_client_kwargs):
        super().__init__(port, token)
        self.http_client = QueuedHTTPClient.get_client(self.host, self.port, **http_client_kwargs)

    async def _do_fetch(self, url, headers):
        request = HTTPRequest(
            query=url,
            headers=headers,
            request_timeout=0.1,
            retries=3,
        )

        # retries 5xx 3 times.
        return await self.http_client.fetch(request, raise_error=False)


class NewStyleHttpClientForTvmDaemon(HttpClientForTvmDaemonBase):
    def __init__(self, port, token):
        super().__init__(port, token)

    async def _do_fetch(self, url, headers):
        async with aiohttp.ClientSession() as session:
            async with session.get(url, headers=headers) as response:
                if response.status == 200:
                    body = await response.text()
                else:
                    body = None

                return HTTPResponse(
                    code=response.status,
                    body=body,
                )


class TvmDaemonError(Exception):
    def __init__(self, code, reason):
        self.code = code
        self.reason = reason
        super().__init__(f'TvmDaemonError({code}, {reason})')


class TvmDaemon:
    """
        This client makes requests to tvm-daemon which must be running on same host.
        For api & detail see https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/.
    """

    def __init__(self, http_client_class, auth_token, port, http_client_kwargs={}):
        self.http_client = http_client_class(port, auth_token, **http_client_kwargs)

    @classmethod
    def _make_path(cls, path, cgi_params={}):
        cgi = '&'.join([f'{key}={value}' for key, value in cgi_params.items() if value])
        if cgi:
            return f'{path}?{cgi}'
        else:
            return path

    @classmethod
    def _load_json(cls, string):
        try:
            return json.loads(string)
        except:
            raise TvmDaemonError(500, 'Got invalid json from tvm-daemon')

    async def _fetch(self, path, headers={}, ok_codes=[200]):
        response = await self.http_client.fetch(path, headers)
        code = response.code
        body = response.body or None

        if isinstance(body, bytes):
            body = body.decode('utf-8')

        if code in ok_codes:
            return code, body
        elif code == 400:
            raise TvmDaemonError(code, f'Invalid url: {body}')
        elif code == 401:
            raise TvmDaemonError(code, f'Invalid token: {body}')
        elif code == 403:
            error_msg = self._load_json(body).get('error')
            raise TvmDaemonError(code, f'Unable to validate ticket: {error_msg}')
        else:
            raise TvmDaemonError(code, 'Tvm-daemon is unconscious')

    async def tickets(self, dsts, src=None):
        path = self._make_path('/tvm/tickets', dict(dsts=','.join(map(str, dsts)), src=src))
        code, body = await self._fetch(path)  # either code == 200 or exception raised
        return self._load_json(body)

    async def keys(self):
        path = self._make_path('/tvm/keys')
        code, body = await self._fetch(path)  # either code == 200 or exception raised
        return body

    async def checksrv(self, ticket, dst=None):
        path = self._make_path('/tvm/checksrv', dict(dst=dst))
        headers = {
            'X-Ya-Service-Ticket': ticket,
        }

        code, body = await self._fetch(path, headers, ok_codes=[200, 403])

        # Returns (True, ...) if ticket is valid.
        return (code == 200, self._load_json(body))

    async def checkusr(self, ticket, override_env=None):
        path = self._make_path('/tvm/checkusr', dict(override_env=override_env))
        headers = {
            'X-Ya-User-Ticket': ticket,
        }

        # Add some exception handling if needed.
        code, body = await self._fetch(path, headers)
        return (code == 200, self._load_json(body))


class TvmDaemonClientError(Exception):
    def __init__(self, reason):
        super().__init__(f'TvmDaemonClientError({reason})')


class TvmDaemonClient:
    def __init__(self, auth_token, port, self_alias=None, http_client_kwargs={}, http_client_type=None):
        """
            self_alias can be None if it is the only client in tvm daemon config.
        """
        self.self_alias = self_alias

        if http_client_type == 'new':
            self.daemon = TvmDaemon(NewStyleHttpClientForTvmDaemon, auth_token, port, http_client_kwargs)
        else:
            self.daemon = TvmDaemon(OldStyleHttpClientForTvmDaemon, auth_token, port, http_client_kwargs)

    async def check_service_ticket(self, ticket):
        ok, info = await self.daemon.checksrv(ticket, dst=self.self_alias)
        if ok:
            return (True, info.get('src'))
        else:
            return (False, info.get('error'))

    async def check_user_ticket(self, ticket):
        return await self.daemon.checkusr(ticket)

    async def get_service_ticket_for(self, dst):
        tickets = await self.daemon.tickets([dst], src=self.self_alias)
        if len(tickets) != 1:
            raise TvmDaemonClientError('TvmDaemon is broken')

        for key, value in tickets.items():
            ticket = value.get('ticket')
            if ticket is None:
                raise TvmDaemonClientError('Unable to get tickets. Error: {}'.format(value.get('error')))
            else:
                return ticket
