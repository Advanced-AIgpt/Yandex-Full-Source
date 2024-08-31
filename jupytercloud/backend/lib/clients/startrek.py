import json

from tornado.web import RequestHandler
from traitlets import Instance, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.util.misc import Url

from .blackbox import BlackboxClient
from .http import AsyncHTTPClientMixin, JCHTTPError
from .tvm import TVMClient


class StartrekClient(LoggingConfigurable, AsyncHTTPClientMixin):
    api_url = Url(default='https://st-api.yandex-team.ru/v2', config=True)
    front_url = Unicode('https://st.yandex-team.ru', config=True)
    app_name = Unicode('jupytercloud', config=True)

    tvm_client = Instance(TVMClient, config=True)

    blackbox = Instance(BlackboxClient)

    parent_handler = Instance(RequestHandler)

    @default('tvm_client')
    def _tvm_client_default(self):
        return TVMClient.instance(parent=self)

    @default('blackbox')
    def _blackbox_default(self):
        return BlackboxClient.instance(parent=self)

    def get_headers(self):
        return (
            super().get_headers() |
            self.tvm_client.get_service_ticket_header('startrek')
        )

    def _encode_params(self, params):
        if params is None:
            return None

        result = {}

        for key in params:
            result[key] = json.dumps(params[key])

        return result

    async def request(self, uri, **kwargs):
        user_ticket_header = await self.blackbox.get_user_ticket_header(self.parent_handler)
        headers = kwargs.pop('headers', {}) | user_ticket_header

        kwargs['params'] = self._encode_params(kwargs.get('params'))

        return await self._raw_request(
            url=self.api_url / uri,
            headers=headers,
            **kwargs,
        )

    async def get_ticket(self, startrek_id):
        try:
            response = await self.request(f'issues/{startrek_id}', method='GET')
        except JCHTTPError as e:
            if e.code == 404:
                return None

            raise

        return json.loads(response.body)

    async def get_tickets(self, startrek_ids):
        assert isinstance(startrek_ids, (list, tuple))

        response = await self.request(
            'issues/_search',
            method='POST',
            data={
                'keys': startrek_ids,
            }
        )

        return json.loads(response.body)

    async def create_link(self, startrek_id, jupyticket_id, relationship):
        await self.request(
            f'issues/{startrek_id}/remotelinks',
            params={
                'notifyAuthor': True,
                'backlink': True,
            },
            data={
                'relationship': relationship,
                'key': jupyticket_id,
                'origin': self.app_name,
            },
        )
