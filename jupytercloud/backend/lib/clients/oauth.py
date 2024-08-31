import json
from urllib.request import quote

from traitlets import Instance, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.clients.blackbox import BlackboxClient
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB


class JupyterCloudOAuth(LoggingConfigurable):
    client_id = Unicode(config=True)
    redirect_host = Unicode(None, allow_none=True, config=True)

    jupyter_cloud_db = Instance(JupyterCloudDB)
    blackbox = Instance(BlackboxClient)

    @default('jupyter_cloud_db')
    def _jupyter_cloud_db_default(self):
        return JupyterCloudDB.instance(parent=self)

    @default('blackbox')
    def _blackbox_default(self):
        return BlackboxClient.instance(parent=self)

    jupyter_cloud_token_name = Unicode('jupyter_cloud')
    login = Unicode()

    _token_cache = {}
    _login_cache = {}

    @classmethod
    def instance(cls, login, **kwargs):
        if login not in cls._login_cache:
            cls._login_cache[login] = cls(
                login=login,
                **kwargs,
            )

        return cls._login_cache[login]

    @property
    def present(self):
        return bool(self.get_jupyter_cloud_token())

    def get_oauth_url(self, state=None, minimalistic=False):
        url = (
            'https://oauth.yandex-team.ru/authorize?response_type=token'
            '&client_id={}&login_hint={}'
            .format(self.client_id, self.login)
        )
        if self.redirect_host:
            redirect_uri = f'https://{self.redirect_host}/hub/yandex_oauth'
            url += f'&redirect_uri={redirect_uri}'

        if state:
            state_str = json.dumps(state)
            url += '&state=' + quote(state_str, safe='')

        if minimalistic:
            url += '&display=popup'

        return url

    def get_jupyter_cloud_token(self):
        if self.login not in self._token_cache:
            self.log.info('geting oauth token for %s', self.login)
            token = self.jupyter_cloud_db.get_oauth_token(
                self.login, self.jupyter_cloud_token_name,
            )

            self._token_cache[self.login] = token

        return self._token_cache[self.login]

    def add_or_update_jupyter_cloud_token(self, value):
        self.log.info('saving oauth token for %s', self.login)
        self.jupyter_cloud_db.add_or_update_token(
            self.login, self.jupyter_cloud_token_name, value,
        )

        self._token_cache[self.login] = value

    def invalidate_token(self):
        self.log.warning('invalidating oauth token for %s', self.login)

        if self.login in self._token_cache:
            del self._token_cache[self.login]

        self.jupyter_cloud_db.drop_oauth_open(
            self.login, self.jupyter_cloud_token_name,
        )

    async def check_scope(self, scope_name, *, userip):
        oauth_token = self.get_jupyter_cloud_token()

        if oauth_token is None:
            return False

        oauth_state = await self.blackbox.oauth(
            userip=userip,
            oauth_token=oauth_token,
        )
        # {'status': {'value': 'INVALID', 'id': 5}, 'error': 'expired_token'}
        token_status = oauth_state.get('status', {}).get('value')
        scope = oauth_state.get('oauth', {}).get('scope', '')
        self.log.debug(
            'user %s have oauth token status %s and scope %r',
            self.login, token_status, scope,
        )
        return token_status == 'VALID' and scope_name in scope
