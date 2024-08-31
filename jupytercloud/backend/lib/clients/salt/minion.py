import asyncio
import asyncssh
import socket
import json
from typing import Callable, Dict

import tenacity
from tornado import httpclient
from traitlets import Instance, Integer, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.lib.clients.redis import RedisClient
from jupytercloud.backend.lib.clients.salt import SaltClient, SaltException
from jupytercloud.backend.lib.clients.ssh import SSHClient, AsyncSSHClient
from jupytercloud.backend.lib.util.logging import LoggingContextMixin


REDIS_EMPTY = '-'


class SaltMinionActionException(SaltException):
    def __init__(self, minion_id, message, pretty_struct=None):
        super().__init__()

        self.minion_id = minion_id
        self.message = message
        self.pretty_struct = pretty_struct

    def __str__(self):
        result = f'[minion {self.minion_id}] {self.message}'
        if self.pretty_struct:
            pretty_struct = json.dumps(self.pretty_struct, indent=4)
            result = f'{result}\nDetails on the response: {pretty_struct}'

        return result


class SaltMinion(LoggingConfigurable, LoggingContextMixin):
    redis_data_bank = Unicode(config=True, default_value=None)
    poll_interval = Integer(5, config=True)

    min_ssh_retry = Integer(30, config=True)
    max_ssh_retry = Integer(60, config=True)

    minion_id = Unicode()
    client = Instance(SaltClient)
    redis = Instance(RedisClient)
    qyp = Instance(QYPClient)

    @default('client')
    def _client_default(self):
        return SaltClient.instance(parent=self)

    @default('redis')
    def _redis_default(self):
        return RedisClient.instance(parent=self)

    @default('qyp')
    def _qyp_default(self):
        return QYPClient.instance(parent=self, config=self.config)

    @property
    def log_context(self):
        return {
            'host': self.minion_id,
        }

    async def wait_until_alive(self):
        self.log.info('wait until is up')

        while True:
            # WARNING: this actually really breaks our salt-postgres base if >1-2 minions are stuck in a boot loop
            # TODO: reconsider needing it at all (with local_key_accepter)
            # TODO: fix consistency_watcher boot-looping minions
            if await self.accept_key():
                if await self.ping():
                    return

            self.log.debug('unresponsive, sleep for %d seconds', self.poll_interval)
            await asyncio.sleep(self.poll_interval)

    async def accept_key(self):
        raw_result = await self.client.wheel(
            'key.accept',
            match=[self.minion_id],
            include_denied=True,
            include_rejected=True,
        )
        result = self._parse_result_single(raw_result)
        wheel_result = result['data']['return']

        minions = wheel_result.get('minions', [])
        minions_denied = wheel_result.get('minions_denied', [])
        minions_rejected = wheel_result.get('minions_rejected', [])

        if self.minion_id in minions_denied:
            raise SaltMinionActionException(self.minion_id, 'key for some reason is denied')

        if self.minion_id in minions_rejected:
            raise SaltMinionActionException(self.minion_id, 'key for some reason is rejected')

        success = self.minion_id in minions

        self.log.debug('result of accepting key is %s', success)

        return success

    async def try_accept_key(self):  # NB: not used
        """Принимает ключи Salt-миньонов.

        Стоит рассказать о текущей схеме работы с ключами.
        Это стоит вынести куда-нибудь в более подходящее место, но пока тут.

        1) Когда мы спавним новую виртуалку, мы ультимативно пытаемся дождаться, пока
           мастер не примет ключ. Если в процессе возникают ошибки -- ничего,
           пользователь увидит ошибку и это хорошо.
        2) Когда мы спавним виртуалку, которая физически уже существует -- ничего
           не меняется, солт-мастер уже принял этот ключ и этот метод вернет True.
        3) Признаком того, что виртуалка "встала" в обоих случаях мы считаем,
           что ключ принялся (что имеет значение для новых виртулок)
           и что до миньона проходит "salt 'minion_id' test.ping".

        Дальше, все становится интереснее:
        1) Текущая инсталляция salt-master намеренно сделана statless.
           Это значит, что при перевыкатке salt-master все ключи миньонов будут потеряны.
           В будущем стоит сделать, чтобы к мастеру на старте монтировалось секурное
           файловое хранилище (потому что salt умеет хранить ключи миньонов только в
           файловой системе)
           Тут надо выбирать, какую систему для этого использовать
           (рассмотреть latency, секьюрность):
           * mds + s3 api + fuse
           * поднять свой etcd
           * redis + fuse
        2) Поэтому, мы во время QYPSpawner.poll пытаемся принять ключ соотвествующего
           minion_id. Раз в минуту по-умолчанию. К слову, это выглядит не очень секьюрно.
           Но во время poll мы должны игнорировать любые ошибки, иначе JH посчитает машинку
           сдохшей и сделает на нее stop, чего мы определенно не хотим.
        3) Миньон может не понять, что он потерял аутентификацию и не попроситься
           на принятие нового ключа.
           Поэтому на миньонах запланирован экшен `event.fire_master "" "ping"` раз в
           пару минут, который положит ключ миньона в minions_pre, и будет успешно принят
           во время QYPSpawner.poll.

        В ряде случаев, ключ миньона может быть отклонен, например, если мы потушили и
        подняли виртуалку. Поэтому мы принимаем и denied ключи.

        Вообще, если мы будем выполнять какой-то массовый экшен в момент переаутентификации
        миьона (сейчас максимальное время -- 3 минуты), то миньон не получит этот экшен.

        Поэтому, с этой хрупкой схемы необходимо переходить на подмонтирование персистентного
        секьюрного хранилища.

        """
        try:
            return await self.accept_key()
        except httpclient.HTTPClientError as e:
            if e.code in (502, 503, 504):
                self.log.debug('network problem while try to accept key: %s', e)
            else:
                self.log.exception('error while try to accept key: %s', e)

        return False

    async def set_grain(self, key, value):
        self.log.info('setting grain %s=%s', key, value)

        # TODO: validate response?
        delete_raw_result = await self.client.local(
            self.minion_id,
            'grains.delkey',
            arg=[key],
        )

        self.log.debug('result of deleting grain %s: %s', key, delete_raw_result)

        refresh = await self.client.local(
            self.minion_id,
            'saltutil.refresh_modules',
            kwarg={'async': False},
            request_all=True,
        )
        self.log.debug('result of refreshing grains: %s', refresh)

        raw_result = await self.client.local(
            self.minion_id,
            'grains.append',
            arg=[key, value],
        )
        result = self._parse_result_single(raw_result)

        self._assert_self_in_result(result)

        self.log.debug('grain result: %s', result)
        grain_values = result[self.minion_id]
        if isinstance(grain_values, str):
            if 'already' in grain_values:
                return

        grain_values = grain_values.get(key, [])
        if not grain_values or grain_values != [value]:
            raise SaltMinionActionException(
                self.minion_id,
                f'wrong grain `{key}` values `{grain_values}` after setting it to `{value}`',
                result,
            )

    async def apply_state(
        self,
        state: str,
        max_poll_interval: int = 15,
        timeout: int = 600,
        failed_state_callback: Callable[Dict[str, str], None] = None,
    ) -> bool:
        state_result = await self.client.apply_state(
            [self.minion_id],
            state,
            max_poll_interval=max_poll_interval,
            timeout=timeout,
            failed_state_callback=failed_state_callback,
        )
        self.log.debug('%s apply_state result: %s', self.minion_id, state_result)
        return state_result.get(self.minion_id) == 'success'

    async def safe_add_to_redis(self):
        """Safe, doesn't erase existing values"""
        self.log.info('Adding minion %s to redis', self.minion_id)
        with await self.redis.client as conn:
            return await conn.hsetnx(self.redis_data_bank, self.minion_id, REDIS_EMPTY)

    async def set_in_redis(self, value=REDIS_EMPTY):
        """Unsafe, may overwrite data"""
        self.log.info('Set redis key %s', self.minion_id)
        with await self.redis.client as conn:
            return await conn.hset(self.redis_data_bank, self.minion_id, value)

    async def check_in_redis(self):
        with await self.redis.client as conn:
            return await conn.hexists(self.redis_data_bank, self.minion_id)

    async def delete_from_redis(self):
        self.log.info('Remove redis key %s', self.minion_id)
        with await self.redis.client as conn:
            return await conn.hdel(self.redis_data_bank, self.minion_id)

    async def get_all_from_redis(self):
        signed, unsigned = {}, {}
        with await self.redis.client as conn:
            async for name, val in conn.ihscan(self.redis_data_bank):
                if val == REDIS_EMPTY:
                    unsigned[name] = val
                else:
                    signed[name] = val
        return signed, unsigned

    def restart(self, restart_timeout=120):
        """Restarts salt-minion using an SSH connection"""

        self.log.debug('restarting minion %s', self.minion_id)
        with SSHClient(host=self.minion_id, parent=self) as client:
            result = client.execute(
                'systemctl restart salt-minion',
                restart_timeout,
                log_output=True,
            )
            result.raise_for_status()

        self.log.debug('Successful restart for %s', self.minion_id)
        return True

    async def get_all_alive(self, users=None) -> list['SaltMinion']:
        all_qyp_vms = await self.qyp.get_user_hosts()

        if users:
            minions = [host for login, host in all_qyp_vms.items() if login in users]
        else:
            minions = list(all_qyp_vms.values())

        pings = await self.client.ping(minions)
        return [SaltMinion(minion_id=host, parent=self) for host, ping in pings.items() if ping]

    async def ping(self) -> bool:
        return self.minion_id in await self.client.ping([self.minion_id], tgt_type='list')

    # *** parsers and validators

    def _parse_result_single(self, raw_result):
        self.log.debug('Parse result single original: %s', raw_result)
        return raw_result['return'][0]

    def _assert_self_in_result(self, result):
        if self.minion_id not in result:
            raise SaltMinionActionException(
                self.minion_id, 'result does not contain this minion id', result,
            )

    async def _try_rewrite_minion_id(self, ssh_client):
        async with ssh_client as connection:
            result = await connection.run('cat /etc/salt/minion_id', check=True)
            written_minion_id = result.stdout.strip()
            if written_minion_id == self.minion_id:
                self.log.debug('/etc/salt/minion_id on minion %s is ok', self.minion_id)
            else:
                self.log.info(
                    '/etc/salt/minion_id on minion %s is wrong (%s) and will be rewrited',
                    self.minion_id, written_minion_id
                )

                await connection.run(f'echo {self.minion_id} > /etc/salt/minion_id', check=True)

                self.log.info('restarting salt-minion process on %s', self.minion_id)
                await connection.run('systemctl restart salt-minion.service', check=True)

    def _log_retry_rewrite_minion_id(self, retry_object, sleep, last_result):
        if not last_result.failed:
            return

        ex = last_result.exception()
        verb, value = 'raised', '%s: %s' % (type(ex).__name__, ex)

        additional = ''
        if isinstance(ex, asyncssh.Error):
            additional = f'\nSSH Error info {ex.code}: {ex.reason}'

        self.log.debug(
            "Retrying rewrite_minion_id for %s in %s seconds as it %s %s.%s",
            self.minion_id,
            sleep,
            verb, value,
            additional
        )

    async def rewrite_minion_id(self, timeout):
        ssh_client = AsyncSSHClient(host=self.minion_id, parent=self)

        # this exceptions signalizes that VM or sshd on VM are not ready
        exceptions_to_retry = (
            socket.gaierror,
            TimeoutError,
            ConnectionRefusedError,
            ConnectionResetError,
            OSError,
        )

        async for attempt in tenacity.AsyncRetrying(
            retry=tenacity.retry_if_exception_type(exceptions_to_retry),
            wait=tenacity.wait_random(min=self.min_ssh_retry, max=self.max_ssh_retry),
            before_sleep=self._log_retry_rewrite_minion_id,
        ):
            with attempt:
                await self._try_rewrite_minion_id(ssh_client)

                # in case of no error we leaving without retry
                return
