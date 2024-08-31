import asyncio
import json

from tornado import web
from tornado.iostream import StreamClosedError

from jupytercloud.backend.handlers.base import JCAPIHandler, JCPageHandler
from jupytercloud.backend.handlers.utils import NAME_RE, admin_or_self
from jupytercloud.backend.settings import parse_settings


SETTINGS_PREFIX = 'settings'
SETTINGS_PENDING_PREFIX = 'settings-pending'


class BaseSettingsPageHandler(JCPageHandler):
    def get_settings_url(self, user):
        return self.get_hub_url(SETTINGS_PREFIX, user.escaped_name)

    def get_pending_url(self, user, *args):
        return self.get_hub_url(SETTINGS_PENDING_PREFIX, user.escaped_name, *args)

    def get_progress_url(self, user):
        return self.get_pending_url(user, 'progress')


class SettingsPageRedirectHandler(BaseSettingsPageHandler):
    @web.authenticated
    async def get(self):
        next_url = self.get_settings_url(self.current_user)
        self.redirect(next_url)


class SettingsPageHandler(BaseSettingsPageHandler):
    async def write_settings_page(
        self, user, spawner, settings_manager, message=None, message_level=None,
    ):
        pending_url = None
        pending_task = None

        # may be 'start', 'stop' and 'check'.
        # 'check' installed only at the start of JH.
        if spawner.pending:
            pending_url = self.get_hub_url('spawn-pending', user.escaped_name)
            pending_task = spawner.pending
        elif settings_manager.in_progress:
            pending_url = self.get_pending_url(user)
            pending_task = 'settings apply'

        html = await self.render_template(
            'settings.html',
            user=user,
            qyp_link=spawner.qyp_link,
            settings_registry=settings_manager.settings_info(),
            pending_url=pending_url,
            pending_task=pending_task,
            action_url=self.get_hub_url(SETTINGS_PREFIX, user.escaped_name),
            message=message,
            message_level=message_level,
        )

        self.write(html)

    @web.authenticated
    @admin_or_self
    async def get(self, name):
        user = self.user_from_username(name)
        spawner = user.spawner
        settings_manager = spawner.settings_manager

        message = None
        message_level = None
        if exc := settings_manager.settings.exception:
            message = f'Previous settings apply failed with exception {exc[-1]}'
            message_level = 'danger'
        else:
            message = 'Previous settings apply was succeed'
            message_level = 'info'

        await self.write_settings_page(
            user, spawner, settings_manager,
            message=message,
            message_level=message_level,
        )

    @web.authenticated
    @admin_or_self
    async def post(self, name):
        user = self.user_from_username(name)
        spawner = user.spawner
        settings_manager = spawner.settings_manager

        if settings_manager.in_progress:
            raise web.HTTPError(400, 'Settings already applying at the moment')

        form_options = {}
        for key, byte_list in self.request.body_arguments.items():
            form_options[key] = [bs.decode('utf8') for bs in byte_list]

        settings = parse_settings(form_options, prefix='setting-')

        if (
            not settings_manager.settings.diff(settings)
            and not settings_manager.settings.exception
        ):
            await self.write_settings_page(
                user, spawner, settings_manager,
                message='Trying to apply settings without a diff',
                message_level='info',
            )
            return

        try:
            await settings_manager.apply(settings)
        except asyncio.TimeoutError:
            next_url = self.get_pending_url(user)
            self.redirect(next_url)
        else:
            await self.write_settings_page(
                user, spawner, settings_manager,
                message='Settings updated succesfully',
                message_level='success',
            )


class SettingsPendingHandler(BaseSettingsPageHandler):
    @web.authenticated
    @admin_or_self
    async def get(self, name):
        user = self.user_from_username(name)
        spawner = user.spawner
        settings_manager = spawner.settings_manager

        if not settings_manager.in_progress:
            raise web.HTTPError(400, 'setting are not applying at the moment')

        html = await self.render_template(
            'settings_pending.html',
            user=user,
            qyp_link=spawner.qyp_link,
            progress_url=self.get_progress_url(user),
            settings_url=self.get_settings_url(user),
        )

        self.write(html)


class SettingsProgressHandler(JCAPIHandler):
    keepalive_interval = 8

    def initialize(self):
        super().initialize()
        self._finish_event = asyncio.Event()

    def on_finish(self):
        self._finish_event.set()

    def get_content_type(self):
        return 'text/event-stream'

    async def keepalive(self):
        """Write empty lines periodically

        to avoid being closed by intermediate proxies
        when there's a large gap between events.
        """
        while not self._finish_event.is_set():
            try:
                self.write('\n\n')
                await self.flush()
            except (StreamClosedError, RuntimeError):
                return

            finish_task = asyncio.create_task(self._finish_event.wait())

            # NB: wait does not raises CancelledError unlike wait_for
            await asyncio.wait(
                [finish_task],
                timeout=self.keepalive_interval,
            )

    async def send_event(self, event):
        try:
            self.write(f'data: {json.dumps(event)}\n\n')
            await self.flush()
        except StreamClosedError:
            self.log.warning('stream closed while handling %s', self.request.uri)
            # raise Finish to halt the handler
            raise web.Finish()

    async def send_failed_event(self, exception):
        text = (
            f'Settings apply failed: {exception!r}'
            if exception else
            'Settings apply failed by unknown reason'
        )

        await self.send_event({
            'progress': 100,
            'failed': True,
            'message': text,
        })

    @web.authenticated
    @admin_or_self
    async def get(self, name):
        self.set_header('Cache-Control', 'no-cache')

        user = self.user_from_username(name)
        spawner = user.spawner
        settings_manager = spawner.settings_manager
        apply_task = settings_manager._apply_task

        if not settings_manager.in_progress:
            # not pending, no progress to fetch
            if exc := settings_manager.future_exception():
                await self.send_failed_event(exc)
                return
            else:
                raise web.HTTPError(
                    400, 'Settings of user %s are not applying at the moment...', name,
                )

        asyncio.create_task(self.keepalive())

        try:
            async for message in settings_manager.iter_messages():
                await self.send_event(message)
        except asyncio.CancelledError:
            pass

        await asyncio.wait([apply_task])

        if exc := settings_manager.future_exception():
            await self.send_failed_event(exc)
            return

        await self.send_event({
            'progress': 100,
            'ready': True,
            'message': 'Settings apply succeed',
            'url': f'/{SETTINGS_PREFIX}',
        })


default_handlers = [
    (f'/{SETTINGS_PREFIX}', SettingsPageRedirectHandler),
    (f'/{SETTINGS_PREFIX}/{NAME_RE}', SettingsPageHandler),
    (f'/{SETTINGS_PENDING_PREFIX}/{NAME_RE}', SettingsPendingHandler),
    (f'/{SETTINGS_PENDING_PREFIX}/{NAME_RE}/progress', SettingsProgressHandler),
]
