import logging
import tornado.gen
import alice.uniproxy.library.auth.tvm2 as tvm
from alice.uniproxy.library.auth.blackbox import BlackboxError, BlackboxType


# ====================================================================================================================
class BlackboxMock(object):
    @classmethod
    def mock_it(cls):
        cls.make_global()

    @classmethod
    def make_global(cls):
        import alice.uniproxy.library.auth.blackbox as bbox
        if not isinstance(bbox._g_blackbox_client, cls):
            bbox._g_blackbox_client = cls()

    def __init__(self, *args, **kwargs):
        super().__init__()
        self._logger = logging.getLogger('mock.blackbox')

    async def ticket4oauth(self, token, ip, port, env=BlackboxType.Public, *args, **kwargs):
        self._logger.info('ticket4oauth token={}'.format(token))
        if 'valid' in token:
            if env == BlackboxType.Public:
                return 'user-valid-oauth-ticket'
            elif env == BlackboxType.Team:
                return 'user-valid-oauth-team-ticket'
        else:
            raise BlackboxError('INVALID_TOKEN', 'INVALID_TOKEN')

    async def ticket4sessionid(self, cookie, ip, origin, port=None, *args, **kwargs):
        self._logger.info('ticket4session cookie={}'.format(cookie))
        if 'invalid' in cookie:
            raise BlackboxError('INVALID_COOKIE', 'INVALID_COOKIE', 403)

        if 'expired' in cookie:
            raise BlackboxError('EXPIRED', 'EXPIRED', 200)

        if 'valid' in cookie:
            if 'yandex-team' in origin:
                return 'user-valid-team-session-ticket'
            else:
                return 'user-valid-session-ticket'
        else:
            raise BlackboxError('INVALID_COOKIE', 'INVALID_COOKIE', 403)

    async def uid4oauth(self, token: str, client_ip: str, client_port: int, *args, **kwargs):
        self._logger.info('uid4oauth token={}'.format(token))
        if 'valid' in token and 'invalid' not in token:
            return '42', False
        else:
            raise BlackboxError('INVALID_TOKEN', 'INVALID_TOKEN')


# ====================================================================================================================
class TVMToolClientMock:
    @classmethod
    def mock_it(cls):
        cls.make_global()

    @classmethod
    def make_global(cls):
        if not isinstance(tvm._g_tvm_tool_client, cls):
            cls().enable()

    def __init__(self):
        self._orig_client = None

    def __enter__(self):
        self.enable()
        return self

    def __exit__(self, *args, **kwargs):
        self.disable()

    def enable(self):
        self._orig_client = tvm._g_tvm_tool_client
        tvm._g_tvm_tool_client = self
        return self

    def disable(self):
        if self._orig_client is not None:
            tvm._g_tvm_tool_client = self._orig_client

    async def service_ticket_for(self, dst, *args, **kwargs):
        return 'valid-ticket-for-' + str(dst)

    async def check_service_ticket(self, ticket):
        await tornado.gen.sleep(0.1)
        if 'invalid' in ticket:
            raise tvm.TVMError(f'ticket \'{ticket}\' is invalid')
