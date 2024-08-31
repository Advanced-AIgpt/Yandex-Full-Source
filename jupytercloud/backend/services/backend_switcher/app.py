"""Turns backend on and off, depending on the flag in DB"""

import asyncio
import os
import subprocess
from concurrent.futures import ThreadPoolExecutor
from urllib.parse import urlparse

import aioredis
from aioredis.sentinel import RedisSentinel
from traitlets import Instance, Integer, List, Unicode, default

from jupytercloud.backend.services.backend_switcher import handlers
from jupytercloud.backend.services.base.app import JupyterCloudApp


class BackendSwitcherApp(JupyterCloudApp):
    # To please JupyterCloud gods:
    jupyterhub_api_url = ''
    jupyterhub_api_token = ''
    jupyterhub_service_prefix = '/services/backend-switcher'
    port = Integer(default_value=8891, config=True)

    supervisor_timeout = Integer(default_value=60, config=True)
    startup_time = Integer(default_value=60, config=True)
    interval = Integer(default_value=1, config=True)
    service_name = Unicode(config=True)
    sentinels = List(Unicode(), config=True)
    sentinel_port = Integer(default_value=26379, config=True)
    redis_password = Unicode(config=True)
    redis_key = Unicode(config=True)

    dc = Unicode()
    sentinel = Instance(RedisSentinel)

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    @default('dc')
    def _dc_default(self):
        return os.environ['DEPLOY_NODE_DC']

    def init_handlers(self):
        super().init_handlers()
        self.handlers.extend(handlers.get_handlers(self.jupyterhub_service_prefix))

    def start(self):
        super().start()
        self.log.info('Starting backend_switcher')
        self.loop = asyncio.get_running_loop()

        asyncio.create_task(self.init())

    async def start_jupyter(self):
        handlers.started = True
        self.supervisorctl_do('start', 'jupyterhub')
        await asyncio.sleep(self.startup_time)
        self.supervisorctl_do('restart', 'unified-agent')

    async def stop_jupyter(self):
        handlers.started = False
        self.supervisorctl_do('stop', 'jupyterhub')

    async def init(self):
        self.sentinel = await aioredis.create_sentinel(
            [(urlparse(url).hostname, self.sentinel_port) for url in self.sentinels],
            password=self.redis_password,
            encoding='utf-8',
        )

        conn = self.sentinel.slave_for(self.service_name)
        chosen = handlers.dc = await conn.get(self.redis_key)

        if chosen == self.dc:
            await self.start_jupyter()
        else:
            await self.stop_jupyter()

        await self.check_dc()

    def supervisorctl_do(self, command, target):
        self.log.info('Doing %s to %s', command, target)
        try:
            subprocess.run(
                ['/opt/venv/bin/supervisorctl', command, target],
                timeout=self.supervisor_timeout,
            )
        except subprocess.CalledProcessError:
            self.log.exception("Command didn't work, error code %d")
        except FileNotFoundError:
            self.log.exception('No supervisorctl found!')

    async def _check_dc(self):
        conn = self.sentinel.slave_for(self.service_name)

        chosen = handlers.dc = await conn.get(self.redis_key)

        if chosen == self.dc and not handlers.started:
            self.log.info('Got %s as chosen DC, turning on', chosen)
            await self.start_jupyter()
        elif chosen != self.dc and handlers.started:
            self.log.info('Got %s as chosen DC, turning off', chosen)
            await self.stop_jupyter()

    async def check_dc(self):
        while True:
            try:
                await self._check_dc()
            except Exception:
                self.log.exception('error while _check_dc')
            await asyncio.sleep(self.interval)


main = BackendSwitcherApp.launch_instance
