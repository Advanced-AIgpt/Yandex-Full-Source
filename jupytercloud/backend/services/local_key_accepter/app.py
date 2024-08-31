"""Approves Salt-master keys for newly spawned minions"""

import asyncio
import json
import re
from pathlib import Path

import aiojobs
from async_lru import alru_cache
from traitlets import Bool, Instance, Integer, List, Unicode, default

from jupytercloud.backend.lib.clients.redis import RedisClient
from jupytercloud.backend.lib.clients.salt import SaltClient
from jupytercloud.backend.lib.clients.salt.minion import SaltMinion
from jupytercloud.backend.services.base.app import JupyterCloudApp


class KeyAccepterApp(JupyterCloudApp):
    # To please JupyterCloud gods:
    jupyterhub_api_url = Unicode(config=True)
    jupyterhub_api_token = Unicode(config=True)
    jupyterhub_service_prefix = '/services/key-accepter'
    port = Integer(default_value=8895, config=True, allow_none=True)

    tags = List(Unicode(), default_value=[r'salt/auth'], config=True)
    key_directory = Instance(Path, args=('/etc/salt/pki/master/minions',), config=True)
    log_all_messages = Bool(False, config=True)

    redis = Instance(RedisClient)

    @default('redis')
    def _redis_default(self):
        return RedisClient.instance(
            parent=self,
            log=self.log,
        )

    salt_client = Instance(SaltClient)

    @default('salt_client')
    def _salt_client_default(self):
        return SaltClient.instance(
            parent=self,
            log=self.log,
        )

    loop = Instance(asyncio.AbstractEventLoop)
    token = Unicode()
    scheduler = Instance(aiojobs.Scheduler)

    @property
    def tags_regex(self):
        tags = [f'^{tag}$' for tag in self.tags]  # in case we want to react to more tags in the future
        return re.compile('|'.join(tags))

    @alru_cache(maxsize=32)
    async def process_auth_event(self, id_, pub):
        self.log.debug('Processing auth event for %s', id_)
        minion = SaltMinion(
            minion_id=id_,
            parent=self,
            client=self.salt_client,
            log=self.log,
        )

        exists = await minion.check_in_redis()
        if exists:
            await minion.accept_key()
            await minion.set_in_redis(pub)
            self.log.debug('Key %s accepted!', id_)
        else:
            self.log.debug('Key %s not in Redis :(', id_)

    async def process_event(self, message):
        try:
            message = message.removeprefix('data:').strip()

            data = json.loads(message)

            tag = data.get('tag', '')

            if self.log_all_messages or self.tags_regex.fullmatch(tag):
                self.log.debug('Processing event with tag=%s: %s', tag, data)

            if self.tags_regex.fullmatch(tag):
                if tag == 'salt/auth':
                    data = data['data']
                    if data['act'] in ('pend', 'denied'):
                        await self.process_auth_event(data['id'], data['pub'])
                else:
                    raise NotImplementedError("Tag expected to be processed, but wasn't")
        except json.decoder.JSONDecodeError:
            self.log.exception('Bad JSON message! %s', message)
        except AttributeError:
            self.log.exception('Wrong message format!')

    async def listen_events(self):
        try:
            async for message in self.salt_client.listen_events_websocket():
                await self.scheduler.spawn(self.process_event(message))
        except Exception:
            self.log.exception('Error listening to events')
        finally:
            await self.scheduler.spawn(self.listen_events())

    async def bootstrap_master(self):
        self.log.debug('Bootstrapping the master')
        abstract_minion = SaltMinion(
            parent=self,
            client=self.salt_client,
            log=self.log,
        )

        signed, unsigned = await abstract_minion.get_all_from_redis()
        self.log.info('Got %s minions from master, %s of them with keys', len(signed) + len(unsigned), len(signed))

        for id_, pub in signed.items():
            (self.key_directory / id_).write_text(pub)

        self.log.info('Successfully written %s minion key files', len(list(self.key_directory.iterdir())))

    async def start_async(self):
        # I need a scheduler because I run a coro on each event
        # and I need to await them somewhere.
        # One can write it on their own, or you can use something already there
        self.scheduler = await aiojobs.create_scheduler()
        await self.bootstrap_master()
        await self.scheduler.spawn(self.listen_events())

    def start(self):
        super().start()

        self.log.info('Starting local_key_accepter')
        self.loop = asyncio.get_running_loop()
        asyncio.create_task(self.start_async())


main = KeyAccepterApp.launch_instance
