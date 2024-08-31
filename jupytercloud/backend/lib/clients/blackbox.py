import functools

from blackboxer import AsyncBlackbox
from blackboxer.exceptions import BlackboxError
from traitlets import Instance, default
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.clients.tvm import TVMClient


__all__ = ['BlackboxClient', 'BlackboxError']


BLACKBOX_TVM_ALIAS = 'blackbox'
USER_TICKET_HEADER = 'X-Ya-User-Ticket'


class BlackboxClient(SingletonConfigurable):
    tvm_client = Instance(TVMClient, config=True)
    blackbox = Instance(AsyncBlackbox, (), {})

    @default('tvm_client')
    def _tvm_client_default(self):
        return TVMClient.instance(parent=self)

    @functools.lru_cache(None)
    def __getattr__(self, name):
        if name.startswith('_'):
            return super().__getattribute__(name)

        obj = getattr(self.blackbox, name)

        if callable(obj):
            @functools.wraps(obj)
            async def wrapper(*args, **kwargs):
                kwargs.setdefault('headers', {})
                kwargs['headers'].update(
                    self.tvm_client.get_service_ticket_header(BLACKBOX_TVM_ALIAS),
                )

                return await obj(*args, **kwargs)

            return wrapper

        return obj

    async def check_auth(self, handler):
        session_id = handler.request.cookies.get('Session_id')

        if session_id is None:
            blackbox_response = {'error': 'Session_id cookie not set'}
        else:
            blackbox_response = await self.sessionid(
                userip=handler.request.remote_ip,
                sessionid=session_id.value,
                host=handler.settings['jupyter_public_host'],
                get_user_ticket='yes',
            )

        return blackbox_response

    async def get_user_ticket(self, handler):
        # raises BlackBoxError
        blackbox_response = await self.check_auth(handler)

        if blackbox_response.get('error') != 'OK':
            return None

        return blackbox_response['user_ticket']

    async def get_user_ticket_header(self, handler):
        user_ticket = await self.get_user_ticket(handler)
        return {USER_TICKET_HEADER: user_ticket}
