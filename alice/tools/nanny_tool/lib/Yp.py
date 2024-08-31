import asyncio
import json
import os
from typing import List, Dict, Tuple
from urllib.parse import urljoin

from aiohttp import ClientSession

from alice.tools.nanny_tool.lib.http_methods import post
from alice.tools.nanny_tool.lib.tools import get_logger, ask_user_ok, fix_macro, clean_exit

log = get_logger(__name__)

AVAILABLE_DC: Tuple[str, str, str] = ('man', 'vla', 'sas')
POD_SETTINGS_JSON: str = 'resources/pod.json'


class Yp:
    def __init__(self):
        self._endpoint = 'https://yp-lite-ui.nanny.yandex-team.ru/api/yplite/'
        self._auth_token = os.environ.get('NANNY_TOKEN')
        self._session = None
        if not self._auth_token:
            log.error("NANNY_TOKEN not set")
            clean_exit(1, self._session)
        self.loop = asyncio.get_event_loop()

    def _make_session(self):
        return ClientSession(headers={'Authorization': f'OAuth {self._auth_token}'})

    def create_pod_set(self, cpu: int, mem: int, disk: int, count: int, service_mask: str, macro: str, dc: List[str],
                       abcid: int = 2728):
        url = urljoin(self._endpoint, 'pod-sets/CreatePodSet/')
        location_settings = []

        if 'all' in dc:
            dc = AVAILABLE_DC

        service_mask = service_mask.replace('_', '-')

        macro = fix_macro(macro)

        print(f'Will deploy x{count} {cpu} cpu, {mem}Mb RAM, {disk}Mb HDD for services')
        for d in dc:
            print(f'  {service_mask}-{d}')
        print(f'''in {", ".join(dc)}

        It will consume {count * cpu / 1000} cores and {count * mem / 1024}Gb RAM''')

        if not ask_user_ok():
            log.error('Aborted by user')
            exit(1)

        with open(POD_SETTINGS_JSON) as j:
            data_template = json.load(j)

        assert all(
            [x in data_template for x in
             ('allocationRequest', 'cluster', 'serviceId')]), f'Bad data {POD_SETTINGS_JSON}'

        for d in dc:
            data = data_template.copy()
            data['allocationRequest']['memoryGuaranteeMegabytes'] = mem
            data['allocationRequest']['rootFsQuotaMegabytes'] = disk
            data['allocationRequest']['vcpuGuarantee'] = cpu
            data['allocationRequest']['workDirQuotaMegabytes'] = disk
            data['allocationRequest']['replicas'] = count
            data['allocationRequest']['networkMacro'] = macro
            data['cluster'] = d.upper()
            data['serviceId'] = f'{service_mask}-{d}'
            data['quotaSettings']['abcServiceId'] = abcid
            location_settings.append(data)

        if len(location_settings) > 0:
            loop = asyncio.get_event_loop()
            loop.run_until_complete(self._order(location_settings, url))
        else:
            log.error('Nothing to do')

    def create_endpoint_sets(self, service_mask: str, dc: List[str]):
        url = urljoin(self._endpoint, 'endpoint-sets/CreateEndpointSet/')
        if 'all' in dc:
            dc = AVAILABLE_DC

        delimiter = '-' if '-' in service_mask else '_'

        id_mask = service_mask.replace('_', '-')
        location_settings = []
        for d in dc:
            location_settings.append(
                {
                    'cluster': d.upper(),
                    'meta': {
                        'id': f'{id_mask}-{d}',
                        'serviceId': f'{service_mask}{delimiter}{d}'
                    },
                    'spec': {
                        'description': None,
                        'podFilter': None,
                        'port': 80,
                        'protocol': 'TCP'
                    }
                }
            )
        loop = asyncio.get_event_loop()
        loop.run_until_complete(self._order(location_settings, url))

    async def _order(self, location_settings: List[Dict], url: str):
        async with self._make_session() as self._session:
            tasks = [asyncio.ensure_future(post(url, json.dumps(x), self._session)) for x in location_settings]
            log.info(f'Running {len(tasks)} tasks')
            for response in await asyncio.gather(*tasks):
                if 'code' in response and response['code'] != 200:
                    log.error(response['code'])
                    log.error(response['message'])
                elif 'code' not in response:
                    log.info(response)
