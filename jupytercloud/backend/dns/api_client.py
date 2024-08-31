import json

from async_lru import alru_cache
from traitlets import Bool, Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.clients.http import AsyncHTTPClientMixin


class DNSApiClient(SingletonConfigurable, AsyncHTTPClientMixin):
    oauth_token = Unicode(config=True)
    account = Unicode(config=True)
    url = Unicode('https://dns-api.yandex.net/v2.3', config=True)
    validate_cert = Bool(True, config=True)

    def get_headers(self):
        return {
            'Content-Type': 'application/json',
            'Accept': 'application/json',
            'X-Auth-Token': self.oauth_token,
        }

    async def request(self, uri, **kwargs):
        parts = [part.strip('/') for part in (self.url, self.account, uri)]
        url = '/'.join(parts)

        result = await self._raw_request(url=url, **kwargs)

        return json.loads(result.body)

    def _parse_zone_info(self, data):
        zones = data.get('zones')
        if not zones:
            raise ValueError(f'failed to fetch zone info; response data: {data}')

        return zones[0]

    @alru_cache(maxsize=32)
    async def get_zone_id(self, name):
        data = await self.request(
            uri=f'/zones?{name}',
            method='GET',
        )

        zone_info = self._parse_zone_info(data)
        zone_id = zone_info.get('uuid')

        if not zone_id:
            raise ValueError(f'failed to fetch zone id for {name}; response data: {data}')

        return zone_id

    async def get_zone_records(self, zone_id):
        data = await self.request(
            uri=f'/zones/{zone_id}/records?limit=100',
            method='GET',
        )
        zone_info = self._parse_zone_info(data)
        records_info = zone_info.get('recordsList') or {}
        total_records = records_info['totalEntries']
        result_records = records_info.get('records') or []

        for offset in range(100, total_records, 100):
            data = await self.request(
                uri=f'/zones/{zone_id}/records?limit=100&offset={offset}',
                method='GET',
            )
            zone_info = self._parse_zone_info(data)
            records_info = zone_info.get('recordsList') or {}
            records = records_info.get('records') or []
            result_records.extend(records)

        return result_records

    async def apply_primitives(self, primitives):
        return await self.request(
            uri='/primitives',
            method='PUT',
            data={
                'primitives': primitives,
            },
            request_timeout=180,
        )
