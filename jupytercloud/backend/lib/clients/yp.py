import asyncio
import functools
import socket
from concurrent.futures import Executor, ThreadPoolExecutor

from traitlets import Instance, List, Unicode, default
from traitlets.config import SingletonConfigurable

import yp.data_model as data_model
from jupytercloud.backend.lib.util.logging import LoggingContextMixin
from yp.client import YpClient as BaseYpClient
from yp.common import YP_PRODUCTION_CLUSTERS


class YPClient(SingletonConfigurable, LoggingContextMixin):
    oauth_token = Unicode(config=True)
    clusters = List(YP_PRODUCTION_CLUSTERS, config=True)
    main_cluster = Unicode(default=True)

    @default('main_cluster')
    def _main_cluster_default(self):
        self_host = socket.gethostname()
        for cluster in self.clusters:
            if cluster in self_host:
                return cluster

        return self.clusters[0]

    client = Instance(BaseYpClient)
    executor = Instance(Executor)
    loop = Instance(asyncio.AbstractEventLoop)

    @property
    def log_context(self):
        return {}

    @default('client')
    def _client_default(self):
        return BaseYpClient(
            address=self.main_cluster,
            config=dict(
                token=self.oauth_token,
            ),
        )

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    async def call_method(self, method, *args, **kwargs):
        @functools.wraps(method)
        def wrapper():
            return method(*args, **kwargs)

        return await self.loop.run_in_executor(
            self.executor,
            wrapper,
        )

    @functools.lru_cache(None)
    def __getattr__(self, name):
        if name.startswith('_'):
            return super().__getattribute__(name)

        obj = getattr(self.client, name)

        if callable(obj):
            @functools.wraps(obj)
            async def wrapper(*args, **kwargs):
                return await self.call_method(obj, *args, **kwargs)

            return wrapper

        return obj


async def get_available_networks(parent, username, network_whitelist):
    client = YPClient.instance(parent=parent)
    request = {
        'user_id': username,
        'object_type': 'network_project',
        'permission': data_model.ACA_USE,
    }

    client.log.debug('fetching available network projects for user %s', username)
    response = await client.get_user_access_allowed_to([request])

    if response:
        networks = response[0].get('object_ids', [])
    else:
        networks = []

    whitelisted = set(networks) & set(network_whitelist)

    client.log.debug(
        'user %s have next available network projects: %s; and whitelisted list: %s',
        username, networks, whitelisted,
    )

    return list(whitelisted)


async def get_available_accounts(parent, username):
    client = YPClient.instance(parent=parent)
    request = {
        'user_id': username,
        'object_type': 'account',
        'permission': data_model.ACA_USE,
    }

    client.log.debug('fetching available network projects for user %s', username)
    response = await client.get_user_access_allowed_to([request])

    accounts = response[0].get('object_ids', []) if response else []

    client.log.debug('user %s have access to accounts: %s', username, accounts)

    return accounts
