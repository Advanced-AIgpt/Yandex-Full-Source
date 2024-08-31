import asyncio
import time
from typing import List

from alice.tools.version_alerts.lib.collector import Collector as VACollector
from alice.tools.version_alerts.lib.common import make_logger, flatten

from alice.tools.quota_monitor.lib.resource_providers.yp_provider import YPProvider

log = make_logger('quota_collector')


class Collector(VACollector):
    def __init__(self, ttl: int):
        self._ttl = ttl
        self._usage_by_providers: List = [
            YPProvider()
        ]
        self._last_gather_time: int = int(time.time())
        self.is_started = False

    async def generate_report(self) -> List:
        return await self.raw_data()

    async def raw_data(self):
        tasks = [x.get_results() for x in self._usage_by_providers]
        result = await asyncio.gather(*tasks)
        return await flatten(result)

    async def gather_and_store_data(self):
        tasks = [x.collect() for x in self._usage_by_providers]
        await asyncio.gather(*tasks)
