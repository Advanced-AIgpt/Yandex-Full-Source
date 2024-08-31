import pathlib
from contextlib import contextmanager
from urllib.parse import quote_plus

from jupyterhub import orm
from jupyterhub.apihandlers.base import APIHandler as BaseAPIHandler
from jupyterhub.handlers.base import BaseHandler
from jupyterhub.utils import url_path_join
from tornado import web

from jupytercloud.backend.lib.clients.abc import ABCClient
from jupytercloud.backend.lib.clients.jupyter import CreateDirectoryError, JupyterClient
from jupytercloud.backend.lib.clients.oauth import JupyterCloudOAuth
from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.lib.clients.redis import RedisClient
from jupytercloud.backend.lib.clients.sandbox import SandboxClient
from jupytercloud.backend.lib.clients.startrek import StartrekClient
from jupytercloud.backend.lib.clients.staff import StaffClient
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB


MAX_INCREASED_PATHS = 10
TOKEN_LIFETIME = 60 * 60


class JCHandlerMixin:
    _abc_client = None
    _qyp_client = None
    _jupyter_cloud_db = None
    _sandbox_client = None
    _startrek_client = None
    _oauth = None
    _redis_client = None
    _staff_client = None

    preferred_interface_arg = '_jupytercloud_interface_'
    preferred_interface_arg_default = 'notebook'

    @property
    def qyp_client(self):
        if self._qyp_client is None:
            self._qyp_client = QYPClient.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._qyp_client

    @property
    def jupyter_cloud_db(self):
        if self._jupyter_cloud_db is None:
            self._jupyter_cloud_db = JupyterCloudDB.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._jupyter_cloud_db

    @property
    def abc_client(self):
        if self._abc_client is None:
            self._abc_client = ABCClient.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._abc_client

    @property
    def sandbox_client(self):
        if self._sandbox_client is None:
            self._sandbox_client = SandboxClient.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._sandbox_client

    @property
    def redis_client(self):
        if self._redis_client is None:
            self._redis_client = RedisClient.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._redis_client

    @property
    def startrek_client(self):
        if self._startrek_client is None:
            self._startrek_client = StartrekClient(
                log=self.log,
                config=self.settings['config'],
                parent_handler=self,
            )

        return self._startrek_client

    @property
    def staff_client(self):
        if self._staff_client is None:
            self._staff_client = StaffClient.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._staff_client

    @property
    def jupyter_public_host(self):
        return self.settings.get('jupyter_public_host', self.hub.ip)

    @staticmethod
    def generate_increased_paths(path, max=MAX_INCREASED_PATHS):
        parts = path.stem.rsplit('-', 1)
        if len(parts) > 1 and parts[1].isdigit():
            base = parts[0]
            current_index = int(parts[1])
        else:
            base = path.stem
            current_index = 0

        return (
            path.with_name(f'{base}-{i}').with_suffix(path.suffix)
            for i in range(current_index + 1, max)
        )

    @contextmanager
    def jupyter_client(self, user):
        api_token = user.new_api_token(
            note=f'temporary token for {self.__class__.__name__}',
            expires_in=TOKEN_LIFETIME,
        )
        orm_token = orm.APIToken.find(self.db, api_token)

        try:
            yield JupyterClient(
                oauth_token=api_token,
                user=user.name,
                log=self.log,
                config=self.settings['config'],
            )
        finally:
            self.db.delete(orm_token)
            self.db.commit()

    @property
    def oauth(self):
        if self._oauth is None:
            self._oauth = self.get_user_oauth(self.current_user)

        return self._oauth

    def get_hub_url(self, *parts):
        return url_path_join(self.hub.base_url, *parts)

    def get_user_oauth(self, user):
        if isinstance(user, str):
            user = self.user_from_username(user)

        return JupyterCloudOAuth.instance(
            log=self.log,
            config=self.settings['config'],
            jupyter_cloud_db=self.jupyter_cloud_db,
            login=user.name,
        )

    def get_user_oauth_token(self, user):
        oauth = self.get_user_oauth(user)
        return oauth.get_jupyter_cloud_token()

    async def _choose_download_path(self, jupyter_client, path):
        options = [path] + list(self.generate_increased_paths(path))

        for option in options:
            if not await jupyter_client.check_path_exists(option):
                return option

        raise web.HTTPError(
            400, f'all paths exists on target server: {options};',
        )

    async def _write_file(self, jupyter_client, path, content):
        target_path = pathlib.PurePath(path)

        try:
            await jupyter_client.create_directory(target_path.parent)
        except CreateDirectoryError as e:
            raise web.HTTPError(400, str(e)) from e

        await jupyter_client.write_file(
            name=target_path.name,
            path=str(target_path),
            type='file',
            format='text',
            content=content,
        )

    async def write_file_for_user(self, user, path, content):
        if isinstance(user, str):
            user = self.user_from_username(user)

        with self.jupyter_client(user) as client:
            target_path = await self._choose_download_path(client, path)
            await self._write_file(client, target_path, content)

        return target_path

    def get_next_url_through_spawn(self, username, url):
        spawned = self.get_argument('spawned', None)
        if spawned:
            raise web.HTTPError(
                500, f'redirected from spawn page but there are still no server for {username}',
            )

        base = self.hub.base_url.rstrip('/')

        # Кидаем человека на страницу с спавн-формой (next_url)
        # По нажатию на spawn человек должен попасть на страницу с прогрессом (pending_url),
        # поэтому дописываем это в next_url.
        # По успешному спавну человек должен попасть обратно вот на этот handler,
        # поэтому записываем эту страницу (this_url) в next внутри pending_url.
        # Получаем: next_url = 'spawn?next= pending?next= this'

        if not url.startswith(base):
            url = base + '/' + url.lstrip('/')
        if '?' in url:
            url += '&spawned=1'
        else:
            url += '?spawned=1'

        target_url = quote_plus(url)
        pending_url = quote_plus(f'{base}/spawn-pending/{username}?next={target_url}')
        return f'{base}/spawn/{username}?next={pending_url}'

    def get_next_url_notebook(self, user, notebook_path):
        return self.get_next_url(
            default=url_path_join(
                self.get_hub_url(),
                user.server_url(),
                'tree',
                str(notebook_path),
            ),
        )


class JCPageHandler(BaseHandler, JCHandlerMixin):
    pass


class JCAPIHandler(BaseAPIHandler, JCHandlerMixin):
    def check_referer(self):
        if self.settings.get('jc_debug') is True:
            return True

        # Оригинальный check_referer проверяет, что referer начинается на https://{host}/hub
        # Но мы делаем запросы с https://{host}/user/{username}
        #
        # XXX: Разобраться, в чем прикол проверять referer через host (это же вопрос только
        # правильно сформированныъ заголовков), и вообще прочитать про CORS

        host = self.request.headers.get('Host').split('/')[0]
        referer = self.request.headers.get('Referer')

        if referer and host and referer.split('://')[1].startswith(f'{host}'):
            return True
        self.log.warning('Host and Referer do not match. Host: %s, Referer: %s', host, referer)

        return super().check_referer()
