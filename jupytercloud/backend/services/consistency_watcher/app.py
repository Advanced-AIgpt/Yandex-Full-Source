"""Ensures that user VMs are running"""

import asyncio
import enum
import logging
import random
from datetime import timedelta

import asyncio_pool
from tornado.httpclient import HTTPError
from traitlets import Any, Dict, HasTraits, Instance, Integer, Unicode, UseEnum, default

from jupytercloud.backend.lib.clients.jupyterhub import JupyterhubClient
from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.lib.qyp.status import VMStatus as QYPVMStatus
from jupytercloud.backend.lib.qyp.vm import QYPVirtualMachine
from jupytercloud.backend.lib.util.misc import HasTraitsReprMixin
from jupytercloud.backend.services.base.app import JupyterCloudApp


@enum.unique
class JHVMStatus(enum.Enum):
    undefined = -1
    not_exists = 0
    not_connected = 1
    spawning = 2
    connected = 3
    stopping = 4
    evacuating = 5
    busy = 6


class VM(HasTraits, HasTraitsReprMixin):
    login = Unicode()
    cluster = Unicode()
    id = Unicode()
    failed_connections = Integer(default_value=0)
    status = UseEnum(JHVMStatus, default_value=JHVMStatus.undefined)
    qyp_vm = Any()

    jupyterhub_client = Instance(JupyterhubClient)

    log = Instance(logging.Logger)
    connect_task = Any(allow_none=True, default_value=None)

    def update(self, qyp_vm, jupyterhub_info):
        self.qyp_vm = qyp_vm
        self.login = qyp_vm.login

        if qyp_vm.is_evacuating:
            new_status = JHVMStatus.evacuating
        elif not jupyterhub_info:
            new_status = JHVMStatus.not_exists
        elif jupyterhub_info['spawning']:
            new_status = JHVMStatus.spawning
        elif jupyterhub_info['stopping']:
            new_status = JHVMStatus.stopping
        elif not jupyterhub_info['have_server']:
            new_status = JHVMStatus.not_connected
        else:
            new_status = JHVMStatus.connected

        if new_status in (
            JHVMStatus.connected,
            JHVMStatus.evacuating,
        ):
            # drop failed_connections due to status change provoked
            # outside this app
            self.failed_connections = 0

        if new_status != self.status:
            self.log.debug(
                'update VM status for user %s from %s to %s',
                self.login, self.status, new_status,
            )

        self.status = new_status

    def can_connect(self):
        return (
            self.status in (
                JHVMStatus.not_exists,
                JHVMStatus.not_connected,
                JHVMStatus.busy,
            )
            and not self.connect_task
        )

    async def _connect(self, spawn_options):
        self.log.debug(
            'start connecting for user %s, %d failed tries so far',
            self.login, self.failed_connections,
        )

        if self.status == JHVMStatus.not_exists:
            await self.jupyterhub_client.new_user(self.login)
            self.log.debug('user %s created at the hub', self.login)

        self.log.info('starting spawn for user %s', self.login)

        failed = False

        async for message in self.jupyterhub_client.spawn(self.login, spawn_options):
            self.log.debug('%s spawn message: %s', self.login, message)
            if message.get('failed'):
                failed = True

        self.log.info(
            '%s spawn for user %s',
            'failed' if failed else 'successfull',
            self.login,
        )

        return not failed

    def try_join(self):
        if self.connect_task and self.connect_task.done():
            fail = False
            try:
                result = self.connect_task.result()

                if not result:
                    self.log.warning('connect task for %s reported fail', self.login)
                    fail = True
            except BaseException as e:
                self.log.error(
                    'failed to start VM for user %s with error %r',
                    self.login, e, exc_info=True,
                )
                fail = True
            finally:
                self.log.debug(
                    'connect task for user %s joined with fail=%s', self.login, fail,
                )
                if fail:
                    self.failed_connections += 1
                self.connect_task = None

    def connect(self, spawn_options):
        assert self.can_connect()

        return self._connect(spawn_options)


class ConsistencyWatcherApp(JupyterCloudApp):
    jupyterhub_service_prefix = '/services/consistency-watcher/'
    qyp = Instance(QYPClient)
    jupyterhub_client = Instance(JupyterhubClient)

    vms = Dict(
        key_trait=Unicode(),
        value_trait=Instance(VM),
        default_value={},
    )

    sync_interval = Integer(120, config=True)

    connect_max_tries = Integer(3, config=True)
    connect_pool_size = Integer(30, config=True)
    connect_pool = None
    active_connects = None

    port = Integer(8893, config=True)

    spawn_options = Dict({'start_existing': '1'}, config=True)

    @default('jupyterhub_client')
    def _jupyterhub_client_default(self):
        return JupyterhubClient.instance(
            parent=self,
            url=self.jupyterhub_api_url,
            oauth_token=self.jupyterhub_api_token,
        )

    @default('qyp')
    def _qyp_default(self):
        return QYPClient.instance(parent=self)

    def start(self):
        super().start()

        self.connect_pool = asyncio_pool.AioPool(size=self.connect_pool_size)

        self.io_loop.add_callback(self.main_loop)

    async def main_loop(self):
        try:
            await self.main()
        except BaseException as e:
            self.log.error(
                'main function failed with unexpected error %r, sleep for %d',
                e, self.sync_interval, exc_info=True,
            )

        self.io_loop.add_timeout(
            timedelta(seconds=self.sync_interval),
            self.main_loop,
        )

    async def main(self):
        # Тут мы обходим все ВМ на тему того, не завершились ли connect-таски.
        # И если таска завершилась, надо забрать ее результат, в частности, для
        # exception-ов и их обработки.
        for vm in self.vms.values():
            vm.try_join()

        try:
            await self.update_vms()
        except HTTPError as e:
            self.log.warning('fail to update vm list due to net error %r', e, exc_info=True)
        except BaseException as e:
            self.log.error('fail to update vm list with unexpected error %r', e, exc_info=True)
        else:
            connect_candidates = await self.get_connect_candidates()

            self.log.info('got %d connect candidates', len(connect_candidates))

            if connect_candidates:
                for vm in connect_candidates:
                    vm.connect_task = self.connect_pool.spawn_n(vm.connect(self.spawn_options))

            self.log.info(
                '%d active connects and %d total',
                self.connect_pool.n_active,
                len(self.connect_pool),
            )

    async def update_vms(self):
        qyp_users = await self.load_qyp_users()
        jh_users = await self.load_jh_users()

        new_vms = 0

        for login, qyp_vm in qyp_users.items():
            vm = self.vms.get(login)
            if not vm:
                new_vms += 1
                vm = self.vms[login] = VM(
                    jupyterhub_client=self.jupyterhub_client,
                    log=self.log,
                )

            jupyterhub_info = jh_users.get(login)
            vm.update(qyp_vm, jupyterhub_info)

        missing_users = set(self.vms) - set(qyp_users)

        for login in missing_users:
            del self.vms[login]

        self.log.info(
            '%d vms total, %d new and %d dropped',
            len(self.vms), new_vms, len(missing_users),
        )

    async def get_connect_candidates(self):
        not_connected_raw = [
            vm for vm in self.vms.values()
            if (
                vm.can_connect() and
                vm.failed_connections < self.connect_max_tries
            )
        ]

        qyp_statuses = await self.load_qyp_statuses(vm.qyp_vm for vm in not_connected_raw)
        not_connected = []
        for vm, status in zip(not_connected_raw, qyp_statuses):
            if status.is_startable:
                not_connected.append(vm)
            else:
                self.log.warning(
                    'user %s vm have wrong status %s which is forbid to start',
                    vm.login, status,
                )

        not_connected.sort(
            key=lambda vm: (vm.failed_connections, random.random()),
        )

        return not_connected

    async def load_jh_users(self):
        jh_users = await self.jupyterhub_client.get_users()

        result = {}

        for user_info in jh_users:
            login = user_info['name']
            result[login] = {
                'spawning': user_info['pending'] == 'spawn',
                'stopping': user_info['pending'] == 'stop',
                'have_server': bool(user_info['servers'].get('')),
            }

        return result

    async def load_qyp_users(self):
        vms = await self.qyp.get_vms_raw_info()
        users = {}
        for vm in vms:
            login = vm['login']

            if login in users:
                self.log.error('user %s have duplicated vms', login)
                continue

            users[login] = QYPVirtualMachine.from_raw_info(self.qyp, vm)

        return users

    async def load_qyp_statuses(self, qyp_vms):
        async def get_vm_status(qyp_vm):
            try:
                return await qyp_vm.get_status(do_retries=False)
            except HTTPError:
                return QYPVMStatus.unknown

        coros = []
        for vm in qyp_vms:
            coros.append(get_vm_status(vm))

        return await asyncio.gather(*coros)


main = ConsistencyWatcherApp.launch_instance
