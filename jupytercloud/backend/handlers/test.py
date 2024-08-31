from jupyterhub.user import User
from jupyterhub.services.service import Service

from .base import JCPageHandler


class WhoAmIHandler(JCPageHandler):
    async def get(self):
        user = self.get_current_user_token()
        admin = False

        if user:
            auth_type = 'jh-oauth-token'
        else:
            user = self.get_current_user_cookie()
            auth_type = 'cookie'

        if not user:
            auth_type = client_type = client_name = 'unauthentificated'
        elif isinstance(user, User):
            client_type = 'user'
            client_name = user.name
            admin = user.admin
        elif isinstance(user, Service):
            client_type = 'service'
            client_name = user.name
            admin = user.admin
        else:
            client_type = client_name = 'unknown'

        self.write({
            'auth_type': auth_type,
            'client_type': client_type,
            'client_name': client_name,
            'admin': admin
        })


default_handlers = [
    ('/test/whoami', WhoAmIHandler),
]
