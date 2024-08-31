import jupyterhub.apihandlers.users
import jupyterhub.user
from jupyterhub.utils import url_path_join


def new_post(self, username):
    return


def new_server_url(self, server_name=''):
    spawner = self.spawners[server_name]

    if (
        spawner and
        spawner.settings_manager and
        spawner.settings_manager.settings.prefer_lab
    ):
        return url_path_join(self.url, 'lab')

    # NB: we a ignoring named servers here

    return self.url


def patch():
    jupyterhub.apihandlers.users.ActivityAPIHandler.post = new_post

    jupyterhub.user.User.server_url = new_server_url
