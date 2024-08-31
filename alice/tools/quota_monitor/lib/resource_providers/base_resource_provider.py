import json
from typing import Dict

import aiohttp
from alice.tools.version_alerts.lib.common import make_logger

log = make_logger(__name__)


class BaseResourceProvider:
    def __init__(self):
        self._collected: Dict = dict()
        self._initialize()
        self._session = None

    def __del__(self):
        if self._session:
            self._session.close()

    def _initialize(self):
        self._provider_url: str = 'localhost'
        self._provider_method: str = 'get'
        self._request_payload: Dict = {}
        self._oauth_token = ''
        self._session: aiohttp.ClientSession() = aiohttp.ClientSession()

    async def get_results(self):
        return await self._dict_to_yasm(self._collected)

    async def collect(self) -> Dict:
        fetched_data = await self.fetch_data()
        return await self._verify_and_parse_data(fetched_data)

    async def fetch_data(self):
        """Fetches data by HTTP"""
        try:
            async with self._session.request(url=self._provider_url,
                                             method=self._provider_method,
                                             json=json.dumps(self._request_payload),
                                             headers={'Authorization': f'OAuth {self._oauth_token}'},
                                             raise_for_status=True) as resp:
                response = resp.json()
        except aiohttp.ContentTypeError:
            log.error(f'Bad response from {self._provider_url}: {response}')
            response = None
        except aiohttp.ClientResponseError:
            log.error(f'Unable to fetch {self._provider_url}: {resp.status}')
            response = None
        return response

    async def _verify_and_parse_data(self, provider_data):
        if not provider_data:
            log.error(f'Returning default from {self._provider_url}')
            return {'quota': 0,
                    'usage': 0}
        data = await self.data_parser(provider_data)
        return data

    async def data_parser(self, provider_data):
        """Parses data provided by fetch_data to unistat-ready list"""
        return [['quota', 0],
                ['usage', 0]]

    @staticmethod
    async def _dict_to_yasm(collected_data):
        return [[k, v] for k, v in collected_data.items()]
