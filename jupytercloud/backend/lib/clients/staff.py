import json

from traitlets import Bool, Instance, Unicode, default
from traitlets.config import SingletonConfigurable
from tornado import web

from .http import AsyncHTTPClientMixin, JCHTTPError
from .tvm import TVMClient


class StaffClient(SingletonConfigurable, AsyncHTTPClientMixin):
    url = Unicode('https://staff-api.yandex-team.ru/v3', config=True)
    validate_cert = Bool(True, config=True)

    tvm_client = Instance(TVMClient, allow_none=True, config=True)

    @default('tvm_client')
    def _tvm_client_default(self):
        return TVMClient.instance(parent=self)

    def get_headers(self):
        headers = super().get_headers()
        headers |= self.tvm_client.get_service_ticket_header('staff_api')
        return headers

    async def request(self, uri, **kwargs):
        url = self.url + '/' + uri.lstrip('/')

        return await self._raw_request(url=url, **kwargs)

    async def get_groups(self, filter):
        return await self.request(
            'groups',
            method='GET',
            params=filter,
        )

    async def get_service_role(self, service_id, role_scope):
        try:
            response = await self.get_groups({
                'parent.service.id': service_id,
                'role_scope': role_scope,
                '_one': 1,
                '_fields': 'id,name,affiliation_counters',
                'type': 'servicerole',
            })
        except JCHTTPError as e:
            if e.code == 404:
                return None
            raise

        return json.loads(response.body)

    async def get_single_username_by_telegram(self, telegram_login):
        response = await self.request('persons', method='GET', params={
            'telegram_accounts.value_lower': telegram_login.lower(),
            '_fields': 'login',
            '_limit': 1
        })

        data = json.loads(response.body)
        if data['total'] < 1:
            raise web.HTTPError(404, f'No people with telegram @{telegram_login}')

        if data['total'] > 1:
            raise web.HTTPError(400, f'Multiple people with telegram @{telegram_login}')

        return data['result'][0]['login']
