import asyncio
import os
from typing import Dict, Tuple, List

from alice.tools.version_alerts.lib.common import make_logger, run_in_executor
from yp.client import YpClient, YpClientError
from yp.common import GrpcUnavailableError

from alice.tools.quota_monitor.lib.resource_providers.base_resource_provider import BaseResourceProvider

log = make_logger(__name__)

_DCS = ('sas', 'man', 'vla', 'iva', 'myt')

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

PRECISION = 2


class YPProvider(BaseResourceProvider):
    def __init__(self):
        super().__init__()

    def _initialize(self):
        self._oauth_token = os.environ.get('YP_TOKEN')
        if self._oauth_token is None:
            log.error('No YP_TOKEN provided')
        self.clients = {x: self._create_client(x) for x in _DCS}
        self._account = 'abc:service:2728'

    def _create_client(self, dc: str):
        if dc not in _DCS:
            log.error(f'DC is not allowed: {dc}')
            return
        try:
            return YpClient(dc, config={'token': self._oauth_token})
        except YpClientError:
            log.error(f'Unable to create client for {dc}')

    async def fetch_data(self) -> Dict:
        async def async_dict(key, coroutine):
            return key, await coroutine

        tasks = {dc: self.fetch_data_for_dc(dc) for dc in self.clients}
        return {key: result
                for key, result in
                await asyncio.gather(
                    *(async_dict(key, coroutine) for key, coroutine in tasks.items()
                      )
                )}

    @run_in_executor
    def fetch_data_for_dc(self, dc: str):
        try:
            return self.clients[dc].get_object('account', self._account,
                                               ['/spec/resource_limits/per_segment',
                                                '/status/resource_usage/per_segment'])
        except GrpcUnavailableError:
            log.error(f'Unable to fetch data from {dc}')
            return []

    async def data_parser(self, provider_data: Dict):
        tasks = []
        for dc in provider_data:
            tasks += await self.data_parser_by_dc(provider_data[dc], dc)
        result = await asyncio.gather(*tasks)
        for item in result:
            self._collected.update(item)

    async def data_parser_by_dc(self, dc_data: List, dc: str):
        tasks = []
        if len(dc_data) != 2:
            log.error(f'Bad data received. Skipping {dc}')
            return []
        quota, usage = dc_data
        available_segments = await _get_available_segments(quota, usage)
        if len(available_segments) == 0:
            return []

        for segment in available_segments:
            tasks.extend([
                self._get_usage(dc, segment, usage[segment], quota[segment], ('cpu',), factor=1 / 1000),
                self._get_usage(dc, segment, usage[segment], quota[segment], ('memory',), factor=1 / GB),
                self._get_usage(dc, segment, usage[segment]['disk_per_storage_class'],
                                quota[segment]['disk_per_storage_class'], ('hdd', 'ssd'), factor=1 / GB),
                self._get_usage(dc, segment, usage[segment]['disk_per_storage_class'],
                                quota[segment]['disk_per_storage_class'], ('hdd', 'ssd'), 'bandwidth', '-io',
                                factor=1 / MB),
            ])
            if 'gpu_per_model' in usage[segment]:
                tasks.extend([self._get_usage(dc, segment, usage[segment]['gpu_per_model'],
                                              quota[segment]['gpu_per_model'], ('gpu_tesla_v100',))])

        return tasks

    @staticmethod
    async def _get_usage(dc: str,
                         segment: str,
                         usage: Dict,
                         quota: Dict,
                         keys: Tuple,
                         data_field: str = 'capacity',
                         stat_appendix: str = '-',
                         factor: float = 1.0) -> Dict:
        result = {}
        stat_appendix = await _normalize_stat_appendix(stat_appendix)
        for key in keys:
            if key in usage and data_field in usage[key]:
                used = usage[key][data_field]
            else:
                used = 0
            if key in quota and data_field in quota[key]:
                given = quota[key][data_field]
            else:
                continue
            percent = await _get_percent(used, given)
            left = given - used
            result.update({
                f'{dc}-{segment}-{key}{stat_appendix}usage_axxx': round(used * factor, PRECISION),
                f'{dc}-{segment}-{key}{stat_appendix}quota_axxx': round(given * factor, PRECISION),
                f'{dc}-{segment}-{key}{stat_appendix}percent_axxx': round(percent, PRECISION),
                f'{dc}-{segment}-{key}{stat_appendix}left_axxx': round(left * factor, PRECISION)
            })
        return result


async def _normalize_stat_appendix(stat_appendix: str) -> str:
    if not stat_appendix.startswith('-'):
        stat_appendix = '-' + stat_appendix
    if not stat_appendix.endswith('-'):
        stat_appendix = stat_appendix + '-'
    return stat_appendix


async def _get_available_segments(quota: Dict, usage: Dict) -> List:
    available_segments = list(usage.keys())
    if len(available_segments) == 0:
        log.error('No available segments in provided data. Skipping')
        return []
    for segment in available_segments.copy():
        if segment not in quota.keys():
            log.error(f'Segment not present in quota: {segment}')
            available_segments.remove(segment)
    return available_segments


async def _get_percent(usage: int, quota: int) -> float:
    if quota == 0:
        return 0
    return usage / quota * 100
