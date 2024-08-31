import json

from async_lru import alru_cache
from traitlets import Bool, Unicode
from traitlets.config import SingletonConfigurable

from .http import AsyncHTTPClientMixin


class ABCClient(SingletonConfigurable, AsyncHTTPClientMixin):
    oauth_token = Unicode(config=True)
    url = Unicode('https://abc-back.yandex-team.ru/api/v4', config=True)
    link_base = Unicode('https://abc.yandex-team.ru', config=True)
    validate_cert = Bool(True, config=True)

    async def request(self, uri, **kwargs):
        url = self.url + '/' + uri.lstrip('/')

        return await self._raw_request(url=url, **kwargs)

    @alru_cache(maxsize=1024)
    async def get_service_name_by_id(self, id):
        response = await self.request(
            uri=f'services/{id}/',
            method='GET',
            params={'fields': 'name'},
            raise_error=False,
        )

        if response.code == 404:
            self.log.warning('fail to found ABC service %s', id)
            return

        return json.loads(response.body).get('name', {})

    def get_service_link(self, service_id):
        return f'{self.link_base.rstrip("/")}/services/{service_id}'

    def parse_account_id(self, account_id):
        # common name is "abc:service:2142" -> Jupyter in the Cloud

        parts = account_id.split(':')
        if parts and len(parts) == 3 and parts[0] == 'abc':
            abc_id = parts[2]
            return abc_id

        return None
