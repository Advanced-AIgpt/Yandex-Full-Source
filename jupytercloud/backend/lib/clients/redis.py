import asyncio
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse

import aioredis
from traitlets import Any, Instance, Integer, List, Unicode, default
from traitlets.config import SingletonConfigurable


class RedisClient(SingletonConfigurable):
    executor = Any()

    @default('executor')
    def _default_executor(self):
        return ThreadPoolExecutor(1)

    sentinel_urls = List(Unicode(), config=True)
    sentinel_name = Unicode(config=True)
    password = Unicode(config=True)
    redis_db = Integer(config=True, default_value=0, help="""Redis DB number, 0 to 15""")
    client = Instance(aioredis.Redis)

    @default('client')
    def _client_default(self):
        sentinel_services = [urlparse(url) for url in self.sentinel_urls]

        # here you need to run an async function in sync context, while another loop is already running
        # because traitlets are synchronous
        # there really isn't any other better way
        return aioredis.Redis(
            self.executor.submit(
                asyncio.run,
                aioredis.sentinel.create_sentinel_pool(
                    [(sentinel.hostname, sentinel.port) for sentinel in sentinel_services],
                    db=self.redis_db,
                    password=self.password,
                    encoding='utf-8',
                    timeout=5,
                ),
            )
            .result()
            .master_for(self.sentinel_name),
        )
