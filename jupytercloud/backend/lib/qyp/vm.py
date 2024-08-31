import asyncio
import enum
import json
import socket

from traitlets import Bool, Dict, Instance, Integer, List, Unicode, observe
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.clients.oauth import JupyterCloudOAuth
from jupytercloud.backend.lib.clients.qyp import QYPClient, QYPException, check_evacuating
from jupytercloud.backend.lib.clients.salt.minion import SaltMinion
from jupytercloud.backend.lib.util.logging import LoggingContextMixin
from jupytercloud.backend.lib.util.misc import cancel_task_safe
from jupytercloud.backend.lib.util.report import ReportMixin

from .instance import GB
from .instance import Instance as BaseInstance
from .instance import InternetAcces, UserPresets
from .status import VMStatus


class QYPWrongVMStatusException(QYPException):
    pass


class InvalidStatusException(QYPException):
    pass


@enum.unique
class VMActionType(enum.Enum):
    """Possible actions to do to a QYP VM.

    Cloned from here:
    https://a.yandex-team.ru/arc/trunk/arcadia/infra/qyp/proto_lib/vmagent_api.proto?rev=5126506#L17
    """

    push_config = 0  # * -> CONFIGURED | EMPTY
    start = 1        # STOPPED | CONFIGURED -> RUNNING
    restart = 2      # RUNNING -> RUNNING
    shutdown = 3     # RUNNING -> RUNNING | STOPPED
    reset = 4        # RUNNING -> RUNNING
    poweroff = 5     # RUNNING -> STOPPED
    hard_reset = 6   # * -> CONFIGURED
    snapshot = 7     # For future use
    rescue = 8       # STOPPED | CONFIGURED -> RUNNING
    backup = 9


class QYPVirtualMachine(BaseInstance, LoggingConfigurable, LoggingContextMixin, ReportMixin):
    slow_stop_poll_interval = Integer(60, config=True)

    cluster = Unicode()
    login = Unicode()
    name_prefix = Unicode()
    short_name_prefix = Unicode()
    oauth = Instance(JupyterCloudOAuth, allow_none=True)
    use_user_token = Bool()
    labels = Dict(default_value=None, allow_none=True)

    cname = Unicode(None, allow_none=True)

    _client = None

    @property
    def client(self):
        if self._client is None:
            self._client = QYPClient(
                parent=self,
                oauth=self.oauth,
                use_user_token=self.use_user_token,
            )

        return self._client

    async def is_running(self):
        return (await self.get_status()).is_running()

    _salt = None

    @property
    def salt(self):
        if not self._salt:
            self._salt = SaltMinion(minion_id=self.host, parent=self)

        return self._salt

    # *** meta
    id = Unicode()

    @observe('login')
    def _login_changed(self, change):
        value = change['new']
        id_ = self.name_prefix + value

        if len(id_) >= 30 and self.short_name_prefix:
            id_ = self.short_name_prefix + value

        if len(id_) >= 30:
            raise QYPException(f'vm name {id_} is too long, it must be less then 30 symbols')

        self.id = id_

    owners = List(Unicode())
    group_ids = List(Unicode())

    # *** spec
    account_id = Unicode()
    node_segment = Unicode()
    network_id = Unicode()

    @property
    def use_nat64(self):
        return self.internet_access != InternetAcces.no_access.name

    @property
    def enable_internet(self):
        return self.internet_access == InternetAcces.ipv4.name

    # QYP backend have validation for this name have to be equal /qemu-persistent
    disk_name = Unicode('/qemu-persistent')

    # *** vm config
    disk_resource = Unicode()

    @property
    def real_mem(self):
        # 1 GB will be used for infrastructure components
        return self.mem - GB

    # *** realization

    @property
    def host(self):
        return self.client.get_vm_host(self.id, self.cluster)

    @property
    def pretty_host(self):
        return (self.cname or self.host).rstrip('.')

    @property
    def link(self):
        return f'https://qyp.yandex-team.ru/vm/{self.cluster}/{self.id}'

    @classmethod
    def instantiate(
        cls,
        *,
        spawner,
        name_prefix,
        short_name_prefix,
        owners,
        group_ids,
        disk_resource,
        network_id,
        use_user_token,
        user_instance_options,
    ):
        instance = UserPresets.from_account(
            have_access=use_user_token,
            account_name=user_instance_options['account_id'],
        ).get(user_instance_options['instance_type'])

        if not instance.io_guarantee:
            instance = instance.with_io(user_instance_options['node_segment'])

        instance_traits = instance.get_safe_trait_values()

        return cls(
            parent=spawner,  # for logging inheritance
            oauth=spawner.oauth,
            login=spawner.user.name,
            name_prefix=name_prefix,
            short_name_prefix=short_name_prefix,
            owners=owners,
            group_ids=group_ids,
            disk_resource=disk_resource,
            network_id=network_id,
            cluster=user_instance_options['cluster'],
            account_id=user_instance_options['account_id'],
            node_segment=user_instance_options['node_segment'],
            use_user_token=use_user_token,
            **instance_traits,
        )

    @classmethod
    def from_raw_info(cls, qyp_client, raw_info):
        meta = raw_info['meta']
        owners = meta['auth']['owners']['logins']
        group_ids = meta['auth']['owners']['groupIds']

        spec = raw_info['spec']
        qemu = spec['qemu']
        disk = qemu['volumes'][0]
        disk_type = disk['storageClass']
        resources = qemu['resourceRequests']

        return cls(
            parent=qyp_client,
            id=raw_info['id'],
            oauth=None,
            name_prefix=qyp_client.vm_name_prefix,
            short_name_prefix=qyp_client.vm_short_name_prefix,
            login=raw_info['login'],
            owners=owners,
            group_ids=group_ids,
            disk_resource=disk['resourceUrl'],
            network_id=qemu['networkId'],
            cluster=raw_info['cluster'],
            account_id=spec['accountId'],
            node_segment=qemu['nodeSegment'],
            use_user_token=False,
            cpu=int(resources['vcpuGuarantee']),
            cpu_limit=int(resources['vcpuLimit']),
            mem=int(resources['memoryGuarantee']),
            disk_size=int(disk['capacity']),
            disk_type=disk_type,
            io_guarantee=int(qemu['ioGuaranteesPerStorage'][disk_type]),
            labels=raw_info['labels'],
        )

    @property
    def meta_config(self):
        return dict(
            id=self.id,
            auth=dict(
                owners=dict(
                    logins=self.owners,
                    groupIds=self.group_ids,
                ),
            ),
        )

    def get_real_cpu_limit(self):
        # We can use cpu overcommiting only in dev segment
        return (
            self.cpu_limit
            if self.node_segment == 'dev' else
            self.cpu
        )

    @property
    def spec_config(self):
        return dict(
            account_id=self.account_id,
            qemu=dict(
                network_id=self.network_id,
                use_nat64=self.use_nat64,
                enable_internet=self.enable_internet,
                node_segment=self.node_segment,
                resource_requests=dict(
                    vcpu_limit=self.get_real_cpu_limit(),
                    vcpu_guarantee=self.cpu,
                    memory_limit=self.mem,
                    # limit and guarantee must be equal,
                    # QYP backend have validation for this
                    memory_guarantee=self.mem,
                ),
                volumes=[
                    dict(
                        name=self.disk_name,
                        capacity=self.disk_size,
                        storage_class=self.disk_type,
                    ),
                ],
                io_guarantees_per_storage={
                    self.disk_type: self.io_guarantee,
                },
            ),
        )

    @property
    def vm_config(self):
        return dict(
            id='empty',  # ???
            vcpu=self.cpu_pretty,
            mem=self.real_mem,
            disk=dict(
                resource=dict(
                    rb_torrent=self.disk_resource,
                ),
                path='/',
                delta_size=0,
                type=1,  # image type = raw
            ),
            type='0',  # linux
            access_info=dict(
                vnc_password='',  # ???
            ),
            autorun=True,
        )

    @property
    def create_config(self):
        return dict(
            type=0,
            meta=self.meta_config,
            spec=self.spec_config,
            config=self.vm_config,
        )

    @property
    def vm_id(self):
        # NB: Some handles wait for vm_id: {'pod_id': self.id}, but some
        # handles wait for vm_id: self.id
        return dict(
            pod_id=self.id,
        )

    @property
    def log_context(self):
        context = {
            'username': self.login,
            'host': self.host,
            'disk_resource': self.disk_resource,
            'cluster': self.cluster,
            'id': self.id,
            'account_id': self.account_id,
            'node_segment': self.node_segment,
            'cpu': self.cpu_pretty,
            'mem': self.mem_pretty,
            'disk_size': self.disk_size_pretty,
            'disk_type': self.disk_type,
        }

        return {k: v for k, v in context.items() if v}

    @property
    def is_evacuating(self):
        return check_evacuating(self.labels or {})

    async def create_and_start(self):
        create_config = self.create_config
        self.log.info('going to create vm with config %s', create_config)

        await self.client.request(
            cluster=self.cluster,
            uri='/api/CreateVm/',
            data=create_config,
        )

    async def get_status(self, do_retries=True):
        result = await self.client.request(
            cluster=self.cluster,
            uri='/api/GetStatus/',
            raise_error=False,
            data=dict(
                vm_id=self.vm_id,  # sic!
            ),
            do_retries=do_retries,
        )

        if result.code == 400:
            return VMStatus.unknown
        if result.code == 404:
            return VMStatus.not_exists

        result.rethrow()

        data = json.loads(result.body)
        raw_status = data['state']['type']

        return VMStatus.status_by_name(raw_status)

    async def get_vm_info(self, do_retries=True):
        result = await self.client.request(
            cluster=self.cluster,
            uri='/api/GetVm/',
            data=dict(
                vm_id=self.id,  # sic!
            ),
            raise_error=False,
            do_retries=do_retries,
        )

        if result.code == 404:
            return None

        result.rethrow()

        data = json.loads(result.body)

        return data

    async def get_ip(self):
        data = await self.get_vm_info()

        vm_status = data['vm']['status']
        ip_allocations = vm_status['ipAllocations']
        for ip_allocation in ip_allocations:
            if ip_allocation['owner'] == 'vm' and ip_allocation['vlanId'] == 'backbone':
                return ip_allocation['address']

        raise QYPException(f'failed to get ip for vm {self.id}')

    async def _make_action(self, action):
        assert isinstance(action, VMActionType)

        await self.client.request(
            cluster=self.cluster,
            uri='/api/MakeAction/',
            data=dict(
                vm_id=self.vm_id,
                action=action.value,
            ),
        )

    async def deallocate(self):
        await self.client.request(
            cluster=self.cluster,
            uri='/api/DeallocateVm/',
            data=dict(
                id=self.id,  # sic!
            ),
            raise_error=False,
        )

    async def stop(self, now):
        if now:
            action = VMActionType.poweroff
        else:
            action = VMActionType.shutdown

        await self._make_action(action)

    async def start(self):
        await self._make_action(VMActionType.start)

    async def wait_for_status(self, statuses, filter, poll_interval):
        statuses = VMStatus.coerce_list(statuses)
        filter = VMStatus.coerce_list(filter)

        self.log.info('waiting vm %s to have %s status', self.id, statuses)

        current_status = await self.get_status()

        while not current_status.is_terminal or current_status in filter:
            self.log.debug(
                'vm %s have status %s, wait for %s or any terminal status except %s',
                self.id, current_status, statuses, filter,
            )
            await asyncio.sleep(poll_interval)
            current_status = await self.get_status()

        if current_status not in statuses:
            self.log.error(
                'vm %s have terminal status %s, but we expected %s',
                self.id, current_status, statuses,
            )
            if current_status == VMStatus.invalid:
                raise InvalidStatusException()

            raise QYPWrongVMStatusException(
                f'vm {self.id} have terminal status {current_status}, but we expected {statuses}',
            )

    async def wait_for_stop(self, poll_interval):
        await self.wait_for_status(
            [VMStatus.stopped, VMStatus.not_exists],
            filter=[VMStatus.running],
            poll_interval=poll_interval,
        )

    async def wait_for_dns(self, poll_interval):
        while True:
            try:
                socket.getaddrinfo(self.pretty_host, 22)
                return True
            except socket.gaierror:
                await asyncio.sleep(poll_interval)

    async def wait_for_or_stop(self, coro):
        wait_for_stop_coro = self.wait_for_stop(self.slow_stop_poll_interval)
        wait_for_stop_task = asyncio.ensure_future(wait_for_stop_coro)
        task = asyncio.ensure_future(coro)

        await asyncio.wait(
            (task, wait_for_stop_task),
            return_when=asyncio.FIRST_COMPLETED,
        )

        if task.done():
            await cancel_task_safe(wait_for_stop_task)
            return task.result()

        if wait_for_stop_task.done():
            await cancel_task_safe(task)
            raise QYPWrongVMStatusException('VM was stopped or removed')

        raise RuntimeError('WTF: neither task or stop task are done')
