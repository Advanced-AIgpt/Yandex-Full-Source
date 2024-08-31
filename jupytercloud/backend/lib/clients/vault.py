import asyncio
import functools
from concurrent.futures import Executor, ThreadPoolExecutor

from library.python.vault_client.client import VaultClient as YavClient
from library.python.vault_client.instances import Production as ProdYavClient
from traitlets import Instance, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.clients.tvm import TVMClient
from jupytercloud.backend.lib.util.logging import LoggingContextMixin


class VaultClient(LoggingConfigurable, LoggingContextMixin):
    user_ticket = Unicode()
    client = Instance(YavClient)
    tvm_client = Instance(TVMClient, config=True)

    jupyter_oauth_token = Unicode(config=True)

    executor = Instance(Executor)
    loop = Instance(asyncio.AbstractEventLoop)

    @property
    def log_context(self):
        return {}

    @default('tvm_client')
    def _tvm_client_default(self):
        return TVMClient.instance(parent=self)

    @default('client')
    def _client_default(self):
        return ProdYavClient(
            authorization=self.jupyter_oauth_token,
            decode_files=True,
            user_ticket=self.user_ticket or None,
            service_ticket=self.tvm_client.get_service_ticket('vault'),
            rsa_auth=False,
        )

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    async def _call_method(self, method, *args, **kwargs):
        @functools.wraps(method)
        def wrapper():
            return method(*args, **kwargs)

        return await self.loop.run_in_executor(
            self.executor,
            wrapper,
        )

    # def __getattr__(self, name):
    #     obj = getattr(self.client, name)
    #
    #     if callable(obj):
    #
    #         @functools.wraps(obj)
    #         async def wrapper(*args, **kwargs):
    #             return await self._call_method(obj, *args, **kwargs)
    #
    #         return wrapper
    #
    #     return obj
