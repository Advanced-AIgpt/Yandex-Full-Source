import asyncio
import json
import time
import traceback

import attr
from traitlets import Instance, Integer, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.db import orm
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB
from jupytercloud.backend.lib.qyp.vm import QYPVirtualMachine
from jupytercloud.backend.lib.util.logging import LoggingContextMixin
from jupytercloud.backend.lib.util.misc import MessageList


class ApplySettingsError(RuntimeError):
    pass


def _setting(
    *,
    type,
    default,
    pillar_name=None,
    title_en,
    description_en=None,
):
    metadata = {
        'pillar_name': pillar_name,
        'title_en': title_en,
        'description_en': description_en,
        'type': type.__name__,
        'default': default,
        'is_setting': True,
    }

    return attr.ib(
        type=type,
        default=default,
        metadata=metadata,
    )


@attr.s(kw_only=True, repr=True)
class Settings:
    prefer_lab = _setting(
        type=bool,
        default=True,
        pillar_name='settings',
        title_en='Use JupyterLab by default',
    )
    taxi_dmp_kernel = _setting(
        type=bool,
        default=False,
        pillar_name='settings',
        title_en='Install Taxi DMP kernel',
    )
    yandex_taxi_atlas_dashboards = _setting(
        type=bool,
        default=False,
        pillar_name='settings',
        title_en='Install yandex-taxi-atlas-dashboards bundle',
        description_en='''Attention! Applying this setting leads
        to jupyter process restart and all running kernels loss''',
    )

    exception = attr.ib(default=None)

    def evolve(self, **changes):
        return attr.evolve(self, **changes)

    def asdict(self, only_settings=True):
        if only_settings:
            return attr.asdict(
                self,
                filter=attr.filters.include(*SETTINGS_ATTRS),
            )
        else:
            return attr.asdict(self)

    def update(self, new):
        if isinstance(new, Settings):
            new = new.asdict()

        for name, value in new.items():
            setattr(self, name, value)

    def diff(self, other):
        one = self.asdict()
        if isinstance(other, Settings):
            other = other.asdict()

        return {
            key: (value, other[key])
            for key, value in one.items()
            if value != other[key]
        }


SETTINGS_REGISTRY = {
    field.name: field.metadata
    for field in attr.fields(Settings)
    if field.metadata.get('is_setting')
}
SETTINGS_ATTRS = {
    field for field in attr.fields(Settings)
    if field.metadata.get('is_setting')
}
PILLARS_REGISTRY = {
    key: metadata['pillar_name']
    for key, metadata in SETTINGS_REGISTRY.items()
    if metadata['pillar_name'] is not None
}


class SettingsManager(LoggingConfigurable, LoggingContextMixin):
    apply_timeout = Integer(10 * 60, config=True)
    slow_apply_timeout = Integer(5, config=True)

    login = Unicode()
    vm = Instance(QYPVirtualMachine, default_value=None, allow_none=True)

    jupyter_cloud_db = Instance(JupyterCloudDB)

    @default('jupyter_cloud_db')
    def _jupyter_cloud_db_default(self):
        return JupyterCloudDB.instance(parent=self)

    settings = Instance(Settings)

    @default('settings')
    def _settings_default(self):
        user = orm.User.find(self.jupyter_cloud_db, name=self.login)
        if user and user.settings:
            settings = Settings(**user.settings)
        else:
            settings = Settings()

        return settings

    @property
    def minion(self):
        if self.vm is None:
            return None
        return self.vm.salt

    @property
    def log_context(self):
        return {
            'login': self.login,
        }

    def settings_info(self):
        result = {}
        for name, info in SETTINGS_REGISTRY.items():
            result[name] = {
                'value': getattr(self.settings, name),
                **info,
            }

        return result

    def _commit_pillars(self):
        assert self.minion

        pillars = {}

        for field_name, pillar_name in PILLARS_REGISTRY.items():
            pillar = pillars.setdefault(pillar_name, {})
            pillar[field_name] = getattr(self.settings, field_name)

        with self.jupyter_cloud_db.transaction():
            for name, value in pillars.items():
                self.jupyter_cloud_db.add_or_update_pillar(
                    self.minion.minion_id,
                    name,
                    json.dumps(value),
                )

    def _commit(self):
        with self.jupyter_cloud_db.transaction():
            user = self.jupyter_cloud_db.add_or_update_user(self.login)

            old_settings = Settings(**(user.settings or {}))

            self.log.info(
                'commit settings with diff %r',
                self.settings.diff(old_settings),
            )

            user.settings = attr.asdict(self.settings)

            if self.minion:
                self._commit_pillars()

    def update_no_apply(self, settings):
        """Part of the main class interface"""

        if self.in_progress:
            raise ApplySettingsError(
                "Can't update settings while settings applying in progress",
            )

        self.settings.update(settings)
        self._commit()

    # Apply mechanic

    _apply_pending = False
    _apply_task = None
    _apply_start_time = None
    _apply_messages = None

    @property
    def in_progress(self):
        return self._apply_pending

    def _apply_is_heavy(self, settings):
        changed = self.settings.diff(settings)

        if self.settings.exception:
            return True

        for key in changed:
            if key in PILLARS_REGISTRY:
                return True

        return False

    async def apply(self, settings):
        """
        Part of the main class interface.
        Raises TimeoutError if applying is too slow for momentary applying,
        but applying continues after.

        """

        if self.in_progress:
            raise RuntimeError('settings already applying')

        self.log.info('start settings apply')

        self._apply_pending = True
        self._apply_messages = MessageList()
        self._apply_start_time = time.perf_counter()

        apply_settings_future = asyncio.wait_for(
            self._apply(settings),
            self.apply_timeout,
        )

        self._apply_task = asyncio.create_task(
            apply_settings_future,
            name=f'apply_settings_{self.login}',
        )

        self._apply_task.add_done_callback(self._clear_apply_settings)

        await asyncio.wait_for(
            asyncio.shield(self._apply_task),
            timeout=self.slow_apply_timeout,
        )

    @property
    def iter_messages(self):
        return self._apply_messages.__aiter__

    def future_exception(self):
        if not self._apply_task or not self._apply_task.done():
            return None

        try:
            exc = self._apply_task.exception()
        except asyncio.CancelledError:
            exc = asyncio.CancelledError()

        return exc

    async def _apply(self, settings):
        self._write_message('Settings apply started', progress=1)

        is_heavy = self._apply_is_heavy(settings)

        self.settings.update(settings)
        self._commit()

        if not is_heavy or not self.minion:
            self._write_message('Fast apply success', progress=99)
            return

        self._write_message('Start pinging minion', progress=10)

        ping = await self.minion.ping()

        if not ping:
            # TODO: restart minion process here instead of raising
            raise ApplySettingsError('salt minion is unresponsive, please, restart VM')

        self._write_message('Start applying states', progress=20)

        await self.vm.wait_for_or_stop(
            self.minion.apply_state('settings'),
        )

        self._write_message('States applyed successfully', progress=99)

    def _write_message(self, text, *, progress=None):
        message = {'message': text}
        if progress:
            message['progress'] = progress

        self._apply_messages.append(message)

    def _clear_apply_settings(self, task):
        exc = self.future_exception()

        delta = time.perf_counter() - self._apply_start_time

        if isinstance(exc, TimeoutError):
            self.log.error(
                'settings apply failed in %f seconds due to timeout',
                self.apply_settings_timeout,
            )
        elif exc:
            self.log.error(
                'settings apply failed in %f seconds',
                delta,
                exc_info=exc,
            )
        else:
            self.log.info('settings applied in %f seconds', delta)

        exc_list = None
        if exc:
            exc_list = traceback.format_exception(type(exc), exc, exc.__traceback__)

        try:
            self.settings.exception = exc_list
            self._commit()
        except asyncio.CancelledError:
            pass
        finally:
            self._apply_pending = False
            self._apply_start_time = None
            self._apply_task = None

            self._apply_messages.close()
            self._apply_messages = None


def parse_settings(settings, prefix=None):
    if prefix:
        settings = {
            k[len(prefix):]: v
            for k, v in settings.items()
            if k.startswith(prefix)
        }

    result = {}
    for setting_name, setting_info in SETTINGS_REGISTRY.items():
        if setting_info['type'] == 'bool':
            if setting_name in settings:
                settings.pop(setting_name)
                result[setting_name] = True
            else:
                result[setting_name] = False

            continue

        if setting_name not in settings:
            raise ValueError(f'missing setting {setting_name} from form data')

        raise NotImplementedError(f'setting type {setting_info["type"]} is not supported')

    if settings:
        raise ValueError(f'unknown setting names: {list(settings.keys())}')

    return result
