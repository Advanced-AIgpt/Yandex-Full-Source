import asyncio
import datetime
import functools
import pprint
import traceback
import uuid

from jupyterhub.spawner import Spawner
from tornado import gen, web
from tornado.ioloop import PeriodicCallback
from traitlets import Dict, Float, Instance, Integer, List, Unicode, UseEnum, default

from jupytercloud.backend.dns.manager import DNSManager
from jupytercloud.backend.env import JupyterCloudEnvironment
from jupytercloud.backend.handlers.backup import BackupConfigurable
from jupytercloud.backend.lib.clients.abc import ABCClient
from jupytercloud.backend.lib.clients.http import JCHTTPError
from jupytercloud.backend.lib.clients.oauth import JupyterCloudOAuth
from jupytercloud.backend.lib.clients.qyp import QYPClient, UnknownClusterError
from jupytercloud.backend.lib.clients.sandbox import SandboxClient
from jupytercloud.backend.lib.clients.yp import get_available_networks
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB
from jupytercloud.backend.lib.qyp.permissions import VMPermissions
from jupytercloud.backend.lib.qyp.quota import GB, UserQuota
from jupytercloud.backend.lib.qyp.status import VMStatus
from jupytercloud.backend.lib.qyp.vm import (
    InvalidStatusException, QYPVirtualMachine, QYPWrongVMStatusException,
)
from jupytercloud.backend.lib.util.exc import DiskResourceException, JupyterCloudException
from jupytercloud.backend.lib.util.logging import LoggingContextMixin
from jupytercloud.backend.settings import SettingsManager

from .form import ExistingVMSpawnerForm, QYPUnavailableSpawnerForm, SpawnerForm, parse_user_options


START_TIMEOUT = 60 * 60


def wrap_traceback_message(func):
    @functools.wraps(func)
    async def wrapper(*args, **kwargs):
        try:
            return await func(*args, **kwargs)
        except Exception:
            message = traceback.format_exc().strip('\n\t ')
            raise web.HTTPError(500, message)

    return wrapper


class StopSpawn(JupyterCloudException):
    pass


class MultipleExistingVMs(JupyterCloudException):
    pass


class ApplyStateFailure(JupyterCloudException):
    def __init__(self, state_name, failed_states):
        self.state_name = state_name
        self.failed_states = failed_states

    def __str__(self):
        msg = f'Failure applying `{self.state_name}` Salt state'
        if self.failed_states:
            msg += ':\n'
            msg += pprint.pformat(self.failed_states, width=120)

        return msg


class QYPSpawner(Spawner, LoggingContextMixin):
    # *** account config

    default_account_user = Unicode(config=True)
    default_account_id = Unicode(config=True)

    # *** VM config

    disk_resource = Unicode(config=True)
    disk_resource_filter = Dict(None, config=True, allow_none=True)
    default_network = Unicode(config=True)
    network_whitelist = List(config=True)

    # *** spawner config

    qyp_poll_interval = Integer(5, config=True)
    dns_poll_interval = Integer(5, config=True)
    progress_poll_interval = Integer(5, config=True)
    poll_interval_jitter = Float(0.5, config=True)

    # NB: gracefull shutdown may be work too slow, up to 5-10 minutes
    stop_timeout = Integer(60 * 10, config=True)
    jupyter_cloud_environment = UseEnum(JupyterCloudEnvironment, config=True)

    # NB: this string change "start server" button to form link at admin page
    options_form = 'YES'

    @default('port')
    def _port_default(self):
        return 8888

    @default('start_timeout')
    def _start_timeout_default(self):
        return START_TIMEOUT

    start_internal_timeout = Integer(START_TIMEOUT - 5, config=True)

    # NB: get_env uses this variable, but whe doesn't want to save any env from head hub service
    env_keep = List([])

    # *** sub clients config

    jupyter_cloud_db = Instance(JupyterCloudDB)

    @default('jupyter_cloud_db')
    def _jupyter_cloud_db_default(self):
        return JupyterCloudDB.instance(parent=self)

    qyp = Instance(QYPClient)

    @default('qyp')
    def _qyp_default(self):
        return QYPClient.instance(parent=self)

    sandbox = Instance(SandboxClient)

    @default('sandbox')
    def _sandbox_default(self):
        return SandboxClient.instance(parent=self)

    abc_client = Instance(ABCClient)

    @default('abc_client')
    def _abc_client_default(self):
        return ABCClient.instance(parent=self)

    dns_manager = Instance(DNSManager)

    @default('dns_manager')
    def _dns_manager_default(self):
        return DNSManager.instance(
            parent=self,
            qyp_client=self.qyp,
        )

    settings_manager = Instance(SettingsManager)

    @default('settings_manager')
    def _settings_manager_default(self):
        return SettingsManager(
            parent=self,
            jupyter_cloud_db=self.jupyter_cloud_db,
            login=self.user.name,
            vm=self.vm,
        )

    permissions = Instance(VMPermissions)

    @default('permissions')
    def _permissions_default(self):
        return VMPermissions(
            parent=self,
            login=self.user.name,
            default_account_id=self.default_account_id,
            abc_client=self.abc_client,
        )

    # *** realization

    @property
    def log_context(self):
        result = {
            'username': self.user.name,
        }
        if self.vm:
            result['host'] = self.vm.host

        if self._uuid:
            result['spawn_uuid'] = self._uuid

        return result

    _oauth = None

    @property
    def oauth(self):
        if self._oauth is None:
            self._oauth = JupyterCloudOAuth(
                parent=self,
                jupyter_cloud_db=self.jupyter_cloud_db,
                login=self.user.name,
            )

        return self._oauth

    vm = None

    @property
    def qyp_link(self):
        if self.vm is not None:
            return self.vm.link
        return None

    def _get_env_string(self):
        env = self.get_env()
        return ','.join(f'{k}="{v}"' for k, v in env.items())

    async def _get_disk_resource(self):
        if self.disk_resource:
            return self.disk_resource

        if self.disk_resource_filter:
            assert 'resource_type' in self.disk_resource_filter
            assert 'attrs' in self.disk_resource_filter
            assert 'environment' in self.disk_resource_filter['attrs']

            self.write_message('Fetch last OS resource from sandbox...', 1)

            resource = await self.sandbox.get_last_resource(
                **self.disk_resource_filter,
            )

            self.write_message(f"Resource {resource['id']} will be used as OS image")
            return resource['skynet_id']

        raise DiskResourceException(
            'missing value for c.QYPSpawner.disk_resource_filter or '
            'c.QYPSpawner.disk_resource',
        )

    # NB: Немного грусти про Spawner.handler.
    # Он устанавливается в SpawnHandler перед тем, как позвать Spawner.get_options_form
    # В конце User.spawn он выставляется обратно в None под finally.
    # Таким образом его как бы можно использовать внутри get_options_form.
    # Но, если в начале Spawner.start происходит исключение, SpawnHandler.post вместо редиректа
    # на SpawnPendingHandler снова рисует страницу спавна, снова вызывая Spawner.get_options_form,
    # но в этот раз уже не устанавливая Spawner.handler!
    # Это откровенный баг, и он все еще присутствует в мастере JH (2020-03-04).
    #
    # Чтобы его обойти, мы устанавливаем _prev_handler, который сбрасывается
    # после успешного старта и используется только в отсутствие self.handler.
    # Из минусов могу обнаружить только то, что он держит объект хендлера от GC.
    _prev_handler = None

    @wrap_traceback_message
    async def get_options_form(self):
        handler = self.handler or self._prev_handler

        user_quota = None
        backup_configurable = None
        existing_vm = None
        exception = None
        tb = None

        uri = handler.request.uri
        oauth_state = {'redirect': uri}
        oauth_url = self.oauth.get_oauth_url(state=oauth_state)

        # TODO: this spaghetti scares me so much
        try:
            existing_vm = await self.get_existing_vm()
        except (JCHTTPError, TimeoutError) as e:
            exception = e
            tb = traceback.format_exc()

        if exception:
            pass  # do nothing more
        elif existing_vm:
            form_class = ExistingVMSpawnerForm

            kwargs = {
                'existing_vm': existing_vm,
                'settings_registry': self.settings_manager.settings_info(),
            }
        else:
            form_class = SpawnerForm

            backup_configurable = BackupConfigurable(
                handler=handler,
                sandbox_client=self.sandbox,
                parent=self,
            )

            user_idm_state = self.jupyter_cloud_db.get_patched_user_idm_state(
                self.user.name,
            )

            get_user_quota_coro = UserQuota.request(
                abc_client=self.abc_client,
                qyp_client=self.qyp,
                user=self.user.name,
                jupyter_user=self.default_account_user,
                jupyter_account_id=self.default_account_id,
                idm_state=user_idm_state,
                have_oauth_token=self.oauth.present,
                oauth_url=oauth_url,
            )
            get_available_networks_coro = get_available_networks(
                parent=self,
                username=self.user.name,
                network_whitelist=self.network_whitelist,
            )

            try:
                user_quota, available_networks = await asyncio.gather(
                    get_user_quota_coro, get_available_networks_coro,
                )
            except JCHTTPError as e:
                user_quota = None
                available_networks = None
                exception = e
                tb = traceback.format_exc()

            kwargs = {
                'user_quota': user_quota,
                'backup_configurable': backup_configurable,
                'available_networks': available_networks,
                'default_network': self.default_network,
                'settings_registry': self.settings_manager.settings_info(),
            }

        if exception:
            form_class = QYPUnavailableSpawnerForm
            kwargs = {
                'exception': exception,
                'traceback': tb.strip('\n\t '),
            }

        kwargs['url'] = uri

        form = form_class(
            parent=self,
            oauth_url=oauth_url,
            template_vars=handler.settings.get('template_vars'),
        )

        if self.handler:
            self._prev_handler = self.handler

        return await form.render(**kwargs)

    async def start(self):
        self._uuid = str(uuid.uuid4())
        self.emit_event('spawn.start')
        try:
            # NB: spawner.start обернут в tornado.gen.with_timeout, но
            # он не канселит таск, а нам нужно, чтобы канселил, поэтому
            # тут заворачиваем в еще один timeout.
            timeouted = asyncio.wait_for(self._start(), timeout=self.start_internal_timeout)

            result = await timeouted
            self.emit_event('spawn.success')

            # magic number to try to wait until jupyterlab will be alive
            await asyncio.sleep(15)

            return result
        except asyncio.TimeoutError as e:
            # NB: jupyterhub снаружи ожидает tornado-вский TimeoutError
            self.log.warning('spawn task for %s is canceled by timeout', self.user.name)
            self.emit_event('spawn.fail.timeout')
            raise gen.TimeoutError('Spawn timeout reached') from e
        except asyncio.CancelledError:
            self.log.warning('spawn task for %s is canceled', self.user.name)
            self.emit_event('spawn.fail.cancel')
            raise
        except StopSpawn:
            self.emit_event('spawn.cancel')
            raise
        except Exception as e:
            if isinstance(e, ApplyStateFailure):
                msg = str(e)
            else:
                msg = traceback.format_exc()

            msg += '\nError while spawning VM, going to stop it'

            self.write_message(msg, error=e)
            self.emit_event('spawn.fail.exception')
            await asyncio.sleep(self.progress_poll_interval + 1)
            raise
        finally:
            self._messages = ()
            self._uuid = None
            self.jupyter_cloud_db.clean_dirty()

    async def _spawn(self):
        host = self.vm.host
        # TODO: handle the case when VM already exists, but it must change it's size
        status = await self.vm.get_status()
        if status.is_not_exists:
            self.write_message(f'VM {host} does not exist, going to create it', 5, duration=5)
            self.emit_event('spawn.spawn.create')
            await self.vm.create_and_start()
        elif status.is_stopped:
            self.write_message(f'VM {host} is stopped, going to start it', 25, duration=5)
            self.emit_event('spawn.spawn.start')
            await self.vm.start()
        elif status.is_running:
            self.emit_event('spawn.spawn.running')
            self.write_message(f'VM {host} is already running, skipping QYP spawning stage')
            # do nothing
        elif status.is_invalid:
            raise InvalidStatusException()
        else:
            raise QYPWrongVMStatusException(
                f'trying to start vm {self.vm.name}, '
                f'but does not known what to do with status {status}',
            )

        # NB: there is wait_for outside will raise TimeoutError after self.start_timeout
        await self.vm.wait_for_status(
            'running',
            filter=['stopped'],
            poll_interval=self.qyp_poll_interval,
        )

    async def _spawn_with_retry(self):
        for i in range(10):
            try:
                await self._spawn()
            except InvalidStatusException:
                if i == 9:
                    raise

                self.write_message(
                    f'VM {self.vm.host} have invalid state, so deallocate it, '
                    f'sleep for 30 seconds and do retry {i + 1}/10',
                )
                await self.vm.deallocate()
                await asyncio.sleep(30)
            else:
                break

    async def _start(self):
        user_options = parse_user_options(
            user_options=self.user_options,
            default_network=self.default_network,
        )

        self.log.info(
            'spawn vm for %s with form options %r',
            self.user.name, user_options,
        )

        if 'force_new' in user_options:
            # old options scheme
            if user_options.get('force_new'):
                existing_vm = await self.get_existing_vm()
                if existing_vm:
                    raise ValueError(
                        'trying to spawn new vm with force_new=True and '
                        f'already existing vm: {existing_vm}',
                    )
            else:
                existing_vm = None
        else:
            existing_vm = await self.get_existing_vm()
            if user_options.get('start_existing'):
                if not existing_vm:
                    raise ValueError('got start_existing option but there no existing VM')
            elif existing_vm:
                raise ValueError(
                    f'there are exising VM but no start_existing option passed: {existing_vm}',
                )

        if existing_vm:
            self.vm = existing_vm
        else:
            disk_resource = await self._get_disk_resource()
            account_id = user_options['instance']['account_id']

            group_ids = await self.permissions.get_group_ids(account_id)
            account_robot_access = await self.permissions.check_account_access(account_id)
            use_user_token = not account_robot_access

            if use_user_token and not self.oauth.present:
                raise ValueError(
                    'trying to spawn in service/personal quota without oauth token obtained',
                )

            self.vm = QYPVirtualMachine.instantiate(
                spawner=self,
                # NB: vm_name_prefix was moved from QYPSpawner to QYPClient to
                # avoid repetition of this trait for using in QYPClient.get_vms_raw_info.
                # However, despite QYPVirtualMachine have QYPClient instance on its own,
                # we pass name_prefix here due to backward compatability:
                # it state stored in JH DB with this field and when we will load this
                # at load_state, this field will popup; also, in case of changing vm_prefix
                # we want to store old prefix for old vms in DB.
                name_prefix=self.qyp.vm_name_prefix,
                short_name_prefix=self.qyp.vm_short_name_prefix,
                owners=self.permissions.get_owners(),
                group_ids=group_ids,
                disk_resource=disk_resource,
                network_id=user_options['network_id'],
                use_user_token=use_user_token,
                user_instance_options=user_options['instance'],
            )

        minion = self.vm.salt
        host = self.vm.host

        # in case of start failing we gonna call .stop and .clear_state,
        # so self.settings_manager.minion will be None
        self.settings_manager.vm = self.vm
        self.settings_manager.update_no_apply(user_options['settings'])

        await minion.set_in_redis()
        await self._spawn_with_retry()

        dns_coro = self.dns_manager.set_user_dns(self.user.name, host)
        dns_task = asyncio.ensure_future(dns_coro)

        self.write_message(f'Wait while {host} loading', 50, duration=5)
        self.emit_event('spawn.loading')

        await self._wait_until_alive()

        self.write_message(f'Settupping environment onto {host}', 75, duration=5)
        self.emit_event('spawn.setup')

        # TODO: make request async
        self.jupyter_cloud_db.add_or_update_pillar(
            minion.minion_id, 'jupyterhub_env', self._get_env_string(),
        )

        failed_states = []

        def failed_state_callback(failed_state):
            nonlocal failed_states
            failed_states.append(failed_state)

        # apply user-dependent states
        salt_coro = minion.apply_state(
            'spawn',
            failed_state_callback=failed_state_callback
        )
        if not await self._wait_for_or_stop(salt_coro):
            raise ApplyStateFailure('spawn', failed_states)

        self.write_message(f'Wait until host {self.vm.pretty_host} will grow to DNS', 90)
        self.emit_event('spawn.dns')

        await dns_task
        self.vm.cname = dns_task.result()
        await self._wait_for_or_stop(
            self.vm.wait_for_dns(self.dns_poll_interval),
        )

        self._prev_handler = None

        self.log.info('successfull spawn for %s', self.user.name)

        return (self.vm.host, self.port)

    async def _wait_until_alive(self):
        minion = self.vm.salt

        # timeout is not really neccessary, but in case of bugs it prevents
        # task leaks
        rewrite_coro = minion.rewrite_minion_id(timeout=self.start_internal_timeout)
        ping_coro = self._wait_for_or_stop(
            minion.wait_until_alive()
        )

        rewrite_task = asyncio.create_task(rewrite_coro)
        ping_task = asyncio.create_task(ping_coro)

        done, pending = await asyncio.wait(
            [rewrite_task, ping_task],
            return_when=asyncio.FIRST_COMPLETED
        )

        if rewrite_task in done:
            await rewrite_task
        else:  # rewrite_task in pending => ping_task in done
            rewrite_task.cancel()

        await ping_task

    async def _wait_for_or_stop(self, coro):
        """Асинхронный хендлер остановки машин при спавне.

        Здесь дожидаемся, что случится первым - отработает coro
        или VM пользователя будет удалена или остановалена.
        Во втором случае прекращаем спавн.
        """
        try:
            return await self.vm.wait_for_or_stop(coro)
        except QYPWrongVMStatusException as e:
            raise StopSpawn(str(e)) from e

    async def stop(self, now=False):
        if not self.vm:
            self.log.error(
                'trying to stop vm for user %s, but spawner is not initialized',
                self.user.name,
            )
            return

        status = await self.vm.get_status()
        if status.is_stopped or status.is_not_exists:
            self.log.warning('no need to stop server %s: it have status %s', self.vm.id, status)
            return

        await self.vm.salt.delete_from_redis()
        await self.vm.stop(now)

        try:
            wait_for_stop_coro = self.vm.wait_for_stop(self.qyp_poll_interval)

            # NB: there is no stop_timeout outsise, it may run forever,
            # so we use wait_for here
            await asyncio.wait_for(wait_for_stop_coro, timeout=self.stop_timeout)

            self.log.info('server %s stopped successfully', self.vm.id)
        except TimeoutError:
            if now:
                self.log.error(
                    'failed to do poweroff stop server %s by timeout %d seconds',
                    self.vm.id, self.stop_timeout,
                )
                raise
            else:
                self.log.warning(
                    'after trying to gracefully shutdown server %s for %d seconds '
                    'send a poweroff signal', self.vm.id, self.stop_timeout,
                )
                await self.stop(now=True)

        finally:
            # see the .ready property below
            self.clear_state()

    @property
    def ready(self):
        # NB: if .stop() raises exeption while start,
        # then for a strange reason spawner counts as ready
        if not self.vm:
            return False
        return super().ready

    _last_poll_status = None

    async def poll(self):
        try:
            status = None
            if self.vm:
                status = await self.vm.get_status(do_retries=False)

                if status.is_running:
                    # server is running, all ok
                    result = None
                    self.update_vm_cname()
                elif status.is_terminal or status.is_not_exists:
                    # server in bad status, return not-None value
                    result = status.value
                else:
                    if status.is_wrong:
                        self.log.error('vm have wrong status')

                    # assume that the Spawner has not finished starting, return "running" value
                    result = None
            else:
                # assume that the Spawner has not been initialized, return "not running" value
                result = 0

            self.log.debug(
                "%s's server polled, which have status %s and return %s as a result of poll",
                self.user.name, status, result,
            )

            self._last_poll_status = status
            return result
        except UnknownClusterError:
            self.log.warning(
                'VM %s locates in unknown DC %s, "turning it off"',
                self.vm.id, self.vm.cluster
            )

            self._last_poll_status = 0
            return 0
        except Exception as e:
            if isinstance(e, web.HTTPError) and e.status_code == 599:
                log_method = self.log.warning
            else:
                log_method = self.log.error

            log_method('unexpected error while poll VM %s', self.vm.id, exc_info=True)

            self._last_poll_status = VMStatus.poll_error
            return None

    def start_polling(self):
        """Start polling periodically for single-user server's running state.

        Callbacks registered via `add_poll_callback` will fire if/when the server stops.
        Explicit termination via the stop method will not trigger the callbacks.
        """

        # NB: we are masking Spawner.start_polling method and adding jitter to PeriodicCallback

        if self.poll_interval <= 0:
            self.log.debug('Not polling server')
            return

        self.log.debug(
            'Polling every %is with jitter %f',
            self.poll_interval,
            self.poll_interval_jitter,
        )

        self.stop_polling()

        self._poll_callback = PeriodicCallback(
            callback=self.poll_and_notify,
            callback_time=1e3 * self.poll_interval,
            jitter=self.poll_interval_jitter,
        )
        self._poll_callback.start()

    async def get_url(self):
        # this url formed this way to be consistent to what JH stores in DB;
        # see app.py/init_spawners/check_spawner of jupyterhub package
        proto = 'https' if self.internal_ssl else 'http'
        return f'{proto}://{self.vm.host}:{self.port}/user/{self.user.name}/'

    def get_state(self):
        if self.vm:
            return self.vm.get_safe_trait_values()
        return None

    def update_vm_cname(self):
        def set_cname(cname):
            if not self.vm:
                # set_cname will call shortly after success poll
                # but it is possible that vm will already stopped
                self.log.warning(
                    'trying to update CNAME to %s, but spawner.vm is None',
                    cname,
                )

                return

            if self.vm.cname == cname:
                return

            self.log.debug(
                'update CNAME info for vm %s = %s from %s',
                self.vm.host, cname, self.vm.cname,
            )
            self.vm.cname = cname

        self.dns_manager.update_cname(self.user.name, set_cname)

    def load_state(self, state):
        if not state:
            return

        # XXX: Temp fix for contaminated states
        # TODO: remove using of get_sage_trait_values to explicit serialization of VM
        state.pop('oauth', None)

        # NB: if we will add new traits to QYPVirtualMachine,
        # here is the place to add defaults for old users
        self.vm = QYPVirtualMachine(
            parent=self,
            oauth=self.oauth,
            **state,
        )

    def clear_state(self):
        super().clear_state()
        self.vm = self.settings_manager.vm = None

    # *** progress

    _messages = ()
    _uuid = None

    def write_message(
        self, message, progress=None, *,
        duration=None, error=None, message_html=None,
    ):
        if not self._messages:
            self._messages = []

        time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        message = f'{time}: {message}'

        if duration:
            message += f' (it will take up to {duration} minutes)'

        if message_html:
            pass
        elif error is not None:
            message_html = f'Spawn failed: <pre>{message}</pre>'
        elif self.vm and self.vm.host in message:
            message_html = message.replace(
                self.vm.host,
                f'<a href="{self.vm.link}" target="_blank">{self.vm.host}</a>',
            )

        self.log.info('spawn message: %r, progress: %r', message, progress)

        self._messages.append((message, message_html, progress, error))

    def emit_event(self, event_name):
        self.log.debug('', extra={'event_name': event_name})

    async def progress(self):
        # NB: if user reload page with spawn progress, this generator func will be
        # called again and we want to yield all the spawn messages again

        i = 0
        while self._spawn_pending:
            for (message, message_html, progress, error) in self._messages[i:]:
                data = dict(
                    message=message,
                )
                if message_html:
                    data['html_message'] = message_html
                if progress:
                    data['progress'] = progress
                if error is not None:
                    data['error'] = repr(error)

                yield data

                i += 1

            await asyncio.sleep(self.progress_poll_interval)

    # *** existing vm info restoration

    def _update_vm_from_info(self, vm, vm_info):
        spec = vm_info['vm']['spec']
        qemu = spec['qemu']
        res = qemu['resourceRequests']

        cpu = int(res['vcpuGuarantee'])
        mem = int(res['memoryGuarantee'])
        disk_size = int(qemu['volumes'][0]['capacity'])
        disk_type = qemu['volumes'][0]['storageClass']

        if disk_size != 300 * GB:
            disk_size_tmp = disk_size // 3
            name = f'cpu{cpu // 1000}_ram{mem // GB}_{disk_type}{disk_size_tmp // GB}'
        else:
            name = f'cpu{cpu // 1000}_ram{mem // GB}_{disk_type}{disk_size // GB}'

        vm.cpu = cpu
        vm.mem = mem
        vm.disk_size = disk_size
        vm.disk_type = disk_type
        vm.name = name

        vm.account_id = spec['accountId']
        vm.node_segment = qemu['nodeSegment']
        vm.use_user_token = vm.account_id != self.default_account_id

        vm.labels = spec.get('labels', {})

    async def get_existing_vm(self):
        existing_vms = []

        for cluster in self.qyp.clusters:
            possible_vm = QYPVirtualMachine(
                parent=self,
                oauth=self.oauth,
                name_prefix=self.qyp.vm_name_prefix,
                short_name_prefix=self.qyp.vm_short_name_prefix,
                login=self.user.name,
                cluster=cluster,
            )

            vm_info = await possible_vm.get_vm_info(do_retries=False)

            if vm_info:
                self._update_vm_from_info(possible_vm, vm_info)
                existing_vms.append(possible_vm)

        if not existing_vms:
            return None

        if len(existing_vms) > 1:
            names = [vm.host for vm in existing_vms]
            raise MultipleExistingVMs(
                f'multiple VMs for one user {self.user.name} found: {names}',
            )

        return existing_vms[0]
