import asyncio
import datetime

from jupyterhub.utils import url_path_join
from tornado import web
from traitlets import Any, Instance, Integer, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.clients.http import JCHTTPError
from jupytercloud.backend.lib.clients.sandbox import SandboxClient
from jupytercloud.backend.lib.qyp.status import VMStatus

from .base import JCPageHandler
from .utils import NAME_RE, admin_or_self


class BackupConfigurable(LoggingConfigurable):
    handler = Any()
    sandbox_client = Instance(SandboxClient)

    resource_type = Unicode('JUPYTER_CLOUD_BACKUP', config=True)
    task_type = Unicode('JUPYTER_CLOUD_BACKUP', config=True)
    task_owner = Unicode('JUPYTER_CLOUD', congig=True)
    max_backups = Integer(10, config=True)
    manual_backup_timeout = Integer(12 * 60 * 60, config=True)
    jupyter_public_host = Unicode(config=True)

    status_run = (
        "TEMPORARY",
        "SUSPENDING",
        "ASSIGNED",
        "EXECUTING",
        "RESUMING",
        "PREPARING",
        "SUSPENDED",
        "SUSPENDING",
        "STOPPING",
        "FINISHING",
        "ENQUEUING",
        "ENQUEUED",
    )
    status_canceable = (
        "ASSIGNED",
        "EXECUTING",
        "PREPARING",
        "ENQUEUING",
        "ENQUEUED",
        "SUSPENDED",
        "SUSPENDING",
        "RESUMING",
    )

    status_fail = ('FAILURE', 'EXCEPTION', 'TIMEOUT')
    status_all = status_run + status_fail
    status_notification = ('SUCCESS',) + status_fail

    @default('jupyter_public_host')
    def _jupyter_public_host_default(self):
        return self.handler.hub.ip

    def get_base_url(self, name):
        # handler may be JupyterHubHandler without our cool methods like .get_hub_url
        return url_path_join(self.handler.hub.base_url, 'backup', name)

    def get_backup_url(self, name):
        return url_path_join(self.get_base_url(name), 'backup')

    def get_restore_url(self, name, resource_id):
        return url_path_join(self.get_base_url(name), 'restore', str(resource_id))

    def is_next_url_restore(self, name):
        next_url = self.handler.get_argument('next', default='')
        restore = url_path_join(self.get_base_url(name), 'restore')
        return restore in next_url

    async def get_tasks(self, name, only_running=True):
        today = datetime.datetime.utcnow()
        if only_running:
            status = self.status_run
        else:
            status = self.status_all

        tasks = []
        raw_tasks = await self.sandbox_client.get_tasks(
            self.task_type,
            input_parameters={
                'username': name,
            },
            status=status,
        )

        for nu, task in enumerate(raw_tasks):
            raw_started = task['time']['created']
            started = datetime.datetime.strptime(raw_started, '%Y-%m-%dT%H:%M:%S.%fZ')
            age_min = int((today - started).total_seconds() // 60)

            if task['input_parameters']['do_backup']:
                task_type = 'Backup'
            elif task['input_parameters']['do_restore']:
                task_type = 'Restore'
            else:
                task_type = 'Unknown'

            status = task['status']

            tasks.append(
                dict(
                    number=nu,
                    id=task['id'],
                    status=status,
                    age_min=age_min,
                    running=status in self.status_run,
                    failed=status in self.status_fail,
                    canceable=status in self.status_canceable,
                    task_type=task_type,
                ),
            )

        return tasks

    async def get_backups(self, name, limit=None):
        limit = limit or self.max_backups

        today = datetime.datetime.utcnow()
        backups = []
        raw_backups = await self.sandbox_client.get_resources(
            self.resource_type,
            attrs={
                'user': name,
            },
            limit=limit,
        )

        for nu, backup in enumerate(raw_backups):
            raw_created = backup['time']['created']
            created = datetime.datetime.strptime(raw_created, '%Y-%m-%dT%H:%M:%SZ')
            age = (today - created).days
            ttl = backup['attributes'].get('ttl')
            expires = None
            if ttl and ttl.isdigit():
                expires = int(ttl) - age

            backups.append(
                dict(
                    number=nu,
                    id=backup['id'],
                    host=backup['attributes']['host'],
                    age=age,
                    expires=expires,
                    size=backup['size'] // 1024 ** 2,
                    restore_url=self.get_restore_url(name, backup['id']),
                ),
            )

        return backups

    def get_notifications(self, users):
        return [{
            'recipients': users,
            'statuses': self.status_notification,
            'transport': 'email',
        }]

    async def do_backup(self, user, host, author):
        result = await self.sandbox_client.create_task(
            self.task_type,
            description=f'Manual backup for {user} started from {self.jupyter_public_host}',
            owner=self.task_owner,
            custom_fields=dict(
                use_latest_sandbox_binary=True,
                username=user,
                do_backup=True,
                do_restore=False,
                spawn_new_vm=False,
                old_host=host,
            ),
            notifications=self.get_notifications([author]),
            kill_timeout=self.manual_backup_timeout,
        )
        task_id = result['id']
        await self.sandbox_client.start_task(task_id)

    async def do_restore(self, user, host, author, resource_id):
        result = await self.sandbox_client.create_task(
            self.task_type,
            description=f'Manual restore for {user} started from {self.jupyter_public_host}',
            owner=self.task_owner,
            custom_fields=dict(
                use_latest_sandbox_binary=True,
                username=user,
                do_backup=False,
                do_restore=True,
                spawn_new_vm=False,
                new_host=host,
                backup_resource=resource_id,
            ),
            notifications=self.get_notifications([author]),
        )
        task_id = result['id']
        await self.sandbox_client.start_task(task_id)

    async def stop_task(self, task_id):
        await self.sandbox_client.stop_task(task_id)


class BackupHandler(JCPageHandler):
    _backup = None

    @property
    def backup(self):
        if self._backup is None:
            self._backup = BackupConfigurable(
                config=self.settings['config'],
                sandbox_client=self.sandbox_client,
                handler=self,
            )

        return self._backup

    async def assert_no_current_tasks(self, name):
        if await self.backup.get_tasks(name):
            raise web.HTTPError(
                400,
                'It is forbidden to do anything '
                'while there are any running backup/restore tasks',
            )

    async def get_existing_vm_status(self, user):
        existing_vm = None
        if user and user.spawner:
            try:
                if existing_vm := await user.spawner.get_existing_vm():
                    return existing_vm, await existing_vm.get_status()
            except JCHTTPError:
                # QYP is unavailable
                self.log.warning('failed to reach QYP', exc_info=True)
                return existing_vm, VMStatus.poll_error

        return None, None

    @web.authenticated
    @admin_or_self
    async def get(self, name):
        user = self.user_from_username(name)

        tasks, backups, (existing_vm, existing_vm_status) = await asyncio.gather(
            self.backup.get_tasks(name, only_running=False),
            self.backup.get_backups(name),
            self.get_existing_vm_status(user),
        )

        ongoing_tasks = any(task['running'] for task in tasks)

        html = await self.render_template(
            'backup.html',
            user=user,
            ongoing_tasks=ongoing_tasks,
            tasks=tasks,
            backups=backups,
            backups_nu=len(backups),
            backup_url=self.backup.get_backup_url(name),
            base_backup_url=self.backup.get_base_url(name),
            existing_vm_status=existing_vm_status,
            existing_vm=existing_vm,
        )

        self.write(html)


class BackupRedirectHandler(BackupHandler):
    @web.authenticated
    async def get(self):
        user = self.current_user
        if user is None:
            raise web.HTTPError(403)

        next_url = self.backup.get_base_url(user.name)
        self.redirect(next_url)


class DoBackupHandler(BackupHandler):
    async def get_backup_host(self, user):
        if user.server and user.server.ip:
            return user.server.ip

        vm, vm_status = await self.get_existing_vm_status(user)

        if not vm or not vm_status.is_running:
            raise web.HTTPError(400, 'trying to backup non-running server')

        return vm.host

    @web.authenticated
    @admin_or_self
    async def get(self, name):
        await self.assert_no_current_tasks(name)

        user = self.user_from_username(name)
        host = await self.get_backup_host(user)

        await self.backup.do_backup(
            user=name,
            host=host,
            author=self.current_user.name,
        )

        next_url = self.get_next_url(
            default=self.backup.get_base_url(name),
        )
        self.redirect(next_url)


class DoRestoreHandler(BackupHandler):
    @web.authenticated
    @admin_or_self
    async def get(self, name, resource_id):
        await self.assert_no_current_tasks(name)

        user = self.user_from_username(name)
        if user.spawner.active:
            host = user.server.ip
            await self.backup.do_restore(
                user=name,
                host=host,
                author=self.current_user.name,
                resource_id=resource_id,
            )

            next_url = self.get_next_url(
                default=self.backup.get_base_url(name),
            )
        else:
            this_url = self.backup.get_restore_url(name, resource_id)
            next_url = self.get_next_url_through_spawn(name, this_url)

        self.redirect(next_url)


class StopHandler(BackupHandler):
    @web.authenticated
    @admin_or_self
    async def get(self, name, task_id):
        await self.backup.stop_task(task_id)

        next_url = self.get_next_url(
            default=self.backup.get_base_url(name),
        )
        self.redirect(next_url)


default_handlers = [
    ('/backup', BackupRedirectHandler),
    (f'/backup/{NAME_RE}', BackupHandler),
    (f'/backup/{NAME_RE}/backup', DoBackupHandler),
    (fr'/backup/{NAME_RE}/restore/(?P<resource_id>\d+)', DoRestoreHandler),
    (fr'/backup/{NAME_RE}/stop/(?P<task_id>\d+)', StopHandler),
]
