"""Chooses Redis instance for Traefik based on availability"""

import asyncio
import subprocess
from concurrent.futures import Executor, ThreadPoolExecutor
from datetime import timedelta
from random import choice, normalvariate

from jinja2 import DebugUndefined, Environment
from redis.sentinel import Sentinel
from traitlets import Instance, Integer, List, Tuple, Unicode, default

from jupytercloud.backend.services.base.app import JupyterCloudApp


class GenerateEndpointsApp(JupyterCloudApp):
    # To please JupyterCloud gods:
    jupyterhub_api_url = ''
    jupyterhub_api_token = ''
    jupyterhub_service_prefix = '/services/endpoint-generator'
    port = Integer(default_value=8001, config=True)

    interval = Integer(1, config=True)
    service_name = Unicode(config=True)
    sentinels = List(Unicode(), config=True)
    sentinel_port = Integer(default_value=26379, config=True)
    template_file = Unicode(config=True)
    endpoint_file = Unicode(config=True)

    restart_time_mean = Integer(default_value=12 * 60 * 60, config=True)
    restart_time_stddev = Integer(default_value=1 * 60 * 60, config=True)

    executor = Instance(Executor)
    loop = Instance(asyncio.AbstractEventLoop)
    sentinel = Instance(Sentinel)
    redis_slaves = List(Tuple(Unicode(), Integer()))
    redis_chosen_slave = Tuple(Unicode(), Integer())

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    def start(self):
        super().start()

        self.sentinel = Sentinel([(url, self.sentinel_port) for url in self.sentinels])
        self.redis_slaves = sorted(self.sentinel.discover_slaves(self.service_name))
        self.redis_chosen_slave = choice(self.redis_slaves)
        self.write_endpoint_file(self.redis_chosen_slave)
        self.restart_traefik()

        self.io_loop.add_callback(self.check_redis_slaves)
        self.io_loop.call_later(
            normalvariate(self.restart_time_mean, self.restart_time_stddev), self.periodic_restarter,
        )

    def periodic_restarter(self):
        self.log.info('Restart traefik due to long uptime')
        self.restart_traefik()

        next_time = normalvariate(self.restart_time_mean, self.restart_time_stddev)
        self.log.debug('Next restart in %s seconds', int(next_time))
        self.io_loop.call_later(next_time, self.periodic_restarter)

    def write_endpoint_file(self, redis_slave):
        with open(self.endpoint_file, 'w') as f:
            content = self.instantiate_template(redis_slave)
            f.write(content)
            self.log.info('Writing %s to %s', content, self.endpoint_file)

    def instantiate_template(self, endpoint):
        endpoint_format = f'[{endpoint[0]}]:{endpoint[1]}'

        env = Environment(undefined=DebugUndefined)  # DebugUndefined leaves undefined templates as is
        with open(self.template_file) as f:
            source = f.read()
        template = env.from_string(source)

        return template.render(redis_endpoint=endpoint_format)

    def restart_traefik(self):
        self.log.info('Restarting traefik')
        try:
            subprocess.run(['supervisorctl', 'restart', 'traefik'], timeout=60)
        except subprocess.CalledProcessError:
            self.log.exception("Restarting didn't work, error code %d")
        except FileNotFoundError:
            self.log.exception('No supervisorctl found!')

    def _check_redis_slaves(self):
        new_slaves = sorted([self.sentinel.discover_master(self.service_name)])

        if new_slaves != self.redis_slaves:
            self.log.warning('Redis slaves have changed! Old: %s, new: %s', self.redis_slaves, new_slaves)
            self.redis_chosen_slave = choice(new_slaves)
            self.write_endpoint_file(self.redis_chosen_slave)
            self.restart_traefik()

        self.redis_slaves = new_slaves

    async def check_redis_slaves(self):
        try:
            await self.loop.run_in_executor(self.executor, self._check_redis_slaves)
        except Exception:
            self.log.exception('error while _check_redis_slaves')

        self.io_loop.add_timeout(timedelta(seconds=self.interval), self.check_redis_slaves)


main = GenerateEndpointsApp.launch_instance
