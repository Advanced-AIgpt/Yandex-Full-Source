from urllib.parse import urlparse
from concurrent.futures import ThreadPoolExecutor
from socket import getfqdn
import asyncio

import aioredis
from traitlets import Any, default, Unicode, Bool, Integer, List

from jupyterhub_traefik_proxy import TKvProxy


class TraefikRedisProxy(TKvProxy):
    executor = Any()

    @default("executor")
    def _default_executor(self):
        return ThreadPoolExecutor(1)

    kv_name = "redis"
    databank = Unicode(config=True, default="data")
    sentinel_name = Unicode(config=True, default=None, allow_none=True)
    sentinel_urls = List(trait=Unicode, config=True, default=None, allow_none=True)
    ca_cert = Unicode(
        config=True,
        allow_none=True,
        default_value=None,
        help="""Redis client root certificates""",
    )
    ssl = Bool(config=True, default_value=False)
    db = Integer(config=True, default_value=0, help="""Redis DB number, 0 to 15""")
    encoding = Unicode(config=True, default_value="utf-8")

    @default("kv_url")
    def _default_kv_url(self):
        return "http://127.0.0.1:6379"

    @default("kv_client")
    def _default_client(self):
        share_kwargs = dict(
            db=self.db,
            password=self.kv_password,
            encoding=self.encoding,
            ssl=self.ssl
        )

        if self.sentinel_name is not None and self.sentinel_urls is not None:
            sentinel_services = [urlparse(url) for url in self.sentinel_urls]

            return aioredis.Redis(self.executor.submit(asyncio.run, aioredis.sentinel.create_sentinel_pool(
                [(sentinel.hostname, sentinel.port) for sentinel in sentinel_services],
                **share_kwargs
            )).result().master_for(self.sentinel_name))
        else:
            return self.executor.submit(asyncio.run, aioredis.create_redis_pool(
                self.kv_url,
                **share_kwargs
            )).result()

    @default("kv_traefik_prefix")
    def _default_kv_traefik_prefix(self):
        return "/traefik/"

    @default("kv_jupyterhub_prefix")
    def _default_kv_jupyterhub_prefix(self):
        return "/jupyterhub/"

    def _define_kv_specific_static_config(self):
        kv_url = urlparse(self.kv_url)

        self.static_config["redis"] = {
            "password": self.kv_password,
            "endpoint": str(kv_url.hostname) + ":" + str(kv_url.port),
            "prefix": self.kv_traefik_prefix,
            "tls.ca": self.ca_cert,
        }

    async def _redis_get(self, key):
        with await self.kv_client as conn:
            return await conn.get(key)

    async def _redis_get_prefix(self, prefix):
        keys = []
        with await self.kv_client as conn:
            async for key in conn.iscan(match=f'{prefix}*'):
                keys.append(key)

            pipe = conn.multi_exec()
            for key in keys:
                pipe.get(key)
            values = await pipe.execute()

        return [{"key": k, "value": v} for k, v in zip(keys, values)]

    async def _kv_atomic_add_route_parts(self, jupyterhub_routespec, target, data, route_keys, rule):
        with await self.kv_client as conn:
            pipe = conn.multi_exec()
            pipe.set(jupyterhub_routespec, target)
            pipe.hset(self.databank, target, data)
            pipe.set(route_keys.service_url_path, target)
            pipe.set(route_keys.router_service_path, route_keys.service_alias)
            pipe.set(route_keys.router_rule_path, rule)

            result = await pipe.execute()

        if result[1] == 0:
            result[1] = 1  # HSET returns 0 upon not updating a field
        return all(result), result

    async def _kv_atomic_delete_route_parts(self, jupyterhub_routespec, route_keys):
        target = await self._redis_get(jupyterhub_routespec)
        if target is None:
            self.log.warning("Route %s doesn't exist. Nothing to delete", jupyterhub_routespec)
            return

        to_delete = [
            jupyterhub_routespec,
            route_keys.service_url_path,
            route_keys.router_service_path,
            route_keys.router_rule_path,
        ]

        with await self.kv_client as conn:
            pipe = conn.multi_exec()
            pipe.delete(*to_delete)
            pipe.hdel(self.databank, target)

            result = await pipe.execute()

        return result[0] == len(to_delete), result

    async def _kv_get_target(self, jupyterhub_routespec):
        return await self._redis_get(jupyterhub_routespec)

    async def _kv_get_data(self, target):
        with await self.kv_client as conn:
            return await conn.hget(self.databank, target)

    async def _kv_get_route_parts(self, kv_entry):
        key = kv_entry["key"]
        target = kv_entry["value"]

        # Strip the "/jupyterhub" prefix from the routespec
        routespec = key.replace(self.kv_jupyterhub_prefix, "")
        data = await self._kv_get_data(target) or "{}"

        return routespec, target, data

    async def _kv_get_jupyterhub_prefixed_entries(self):
        return await self._redis_get_prefix(self.kv_jupyterhub_prefix)

    async def add_hub_route(self, hub):
        host = f"http://{getfqdn()}:{hub.port}"

        self.log.info("Adding default route for Hub: %s => %s (modified!)", hub.routespec, host)
        return await self.add_route(hub.routespec, host, {'hub': True})
