from urllib.parse import quote_plus

from jupyterhub.auth import Authenticator
from jupyterhub.handlers import BaseHandler
from jupyterhub.utils import url_path_join
from tornado import web
from traitlets import Instance, default

from jupytercloud.backend.lib.clients.blackbox import BlackboxClient, BlackboxError


PASSPORT_URL = 'https://passport.yandex-team.ru/auth?retpath={}'


class YandexBlackboxHandler(BaseHandler):
    async def get(self, *args, **kwargs):
        user_info = await self.authenticator.get_authenticated_user(self, None)

        if user_info is None:
            retpath_url = self.request.full_url()
            redirect_url = PASSPORT_URL.format(quote_plus(retpath_url))
            self.redirect(redirect_url)
            return

        user = self.user_from_username(user_info['name'])
        self.set_login_cookie(user)

        if hasattr(BaseHandler, 'get_next_url'):
            next_url = super().get_next_url(user)
        else:
            next_url = url_path_join(self.hub.server.base_url, 'home')

        self.redirect(next_url)


class YandexBlackboxAuthenticator(Authenticator):
    auto_login = True
    login_service = 'Yandex Passport'
    # auth_refresh_age = 180
    # refresh_pre_spawn = True

    blackbox = Instance(BlackboxClient)

    @default('blackbox')
    def _blackbox_default(self):
        return BlackboxClient.instance(parent=self)

    def login_url(self, base_url):
        return url_path_join(base_url, 'blackbox')

    def get_handlers(self, app):
        return [(r'/blackbox', YandexBlackboxHandler)]

    async def check_auth(self, handler):
        """Check if session_id cookie is valid

        Asks Blackbox to verify the cookie.

        Args:
            handler: the current request handler
        Returns:
            auth_data (dict or None):
                Return **dict** of auth data if cookie is valid.
                This dict will have the same structure as .authenticate()
                It will include login and user-ticket.
        """
        assert handler is not None

        try:
            blackbox_response = await self.blackbox.check_auth(handler)
        except BlackboxError:
            self.log.error('Blackbox error', exc_info=True)
            raise web.HTTPError(500, 'Blackbox error')

        if blackbox_response.get('error') != 'OK':
            self.log.info('Bad auth: %s', blackbox_response.get('error'))
            return None

        return {
            'name': blackbox_response['login'],
            'auth_state': {
                'user_ticket': blackbox_response['user_ticket'],
            },
        }

    async def authenticate(self, handler, data):
        return await self.check_auth(handler)

    async def refresh_user(self, user, handler=None):
        # this does not log the user out, but removes his user_ticket eventually
        # to use the user_ticket, call `.check_auth()` directly
        return {
            'auth_state': {},
        }
