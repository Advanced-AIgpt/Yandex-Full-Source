import argparse
import asyncio
import os

import uvloop
import yaml
import yappi
from jupyterhub import orm
from jupyterhub.app import JupyterHub
from jupyterhub.metrics import RUNNING_SERVERS, TOTAL_USERS
from sqlalchemy.orm import selectinload
from traitlets import Instance, List, Unicode, default

from jupytercloud.backend.handlers.patch import patch as patch_handlers
from jupytercloud.backend.lib.clients.salt.minion import SaltMinion
from jupytercloud.backend.lib.util.format import pretty_json
from jupytercloud.backend.lib.util.metrics.configurable import MetricsConfigurableMixin
from jupytercloud.backend.lib.util.misc import Url
from jupytercloud.backend.static import WebpackAssets


class JC(JupyterHub, MetricsConfigurableMixin):
    config_file = Unicode('backend_config.py', config=True)
    unified_agent_service_config_file = Unicode('ua_service_config.yaml', config=True)
    webpack_assets_url = Url(None, allow_none=True, config=True)
    external_js = List(Unicode(), default=[], config=True)

    webpack_assets = Instance(WebpackAssets)

    @default('webpack_assets')
    def _webpack_assets_default(self):
        return WebpackAssets(
            webpack_assets_url=self.webpack_assets_url,
            external_js=self.external_js,
        )

    def init_tornado_settings(self):
        self.template_vars['webpack_assets'] = self.webpack_assets

        super().init_tornado_settings()

        self.tornado_settings['jinja2_env'].policies['json.dumps_function'] = pretty_json
        self.tornado_settings['jinja2_env_sync'].policies['json.dumps_function'] = pretty_json

        self.set_jupyter_cloud_request_logger()

    def init_handlers(self):
        super().init_handlers()
        handlers = self.add_url_prefix(self.hub_prefix, self.webpack_assets.generate_handlers())
        self.handlers[-3:-3] = handlers  # NB: last handlers are like (*. -> 404), so we must insert our handler before

    def write_unified_agent_config(self):
        routes = []
        for service in self.services:
            if 'url' in service:
                routes.append({
                    'input': {
                        'plugin': 'metrics_pull',
                        'config': {
                            'url': f'{service["url"]}/services/{service["name"]}/solomon',
                            'format': {'solomon_json': {}},
                            'metric_name_label': 'sensor',
                            'project': 'jupyter-cloud',
                            'service': service['name'],
                        },
                    },
                    'channel': {
                        'channel_ref': {
                            'name': 'solomon_output',
                        },
                    },
                })

        with open(self.unified_agent_service_config_file, mode='w') as s:
            s.write(yaml.dump({'routes': routes}))

    @classmethod
    def parse_pre_init_options(cls, argv):
        parser = argparse.ArgumentParser(add_help=False)

        # OBSOLETE OPTION
        parser.add_argument(
            '--patch-activity-handler', action='store_true', default=os.getenv('JC_PATCH_ACTIVITY_HANDLER') == '1',
        )

        parser.add_argument('--enable-profiling', action='store_true', default=os.getenv('JC_PROFILE') == '1')

        return parser.parse_known_args(argv)

    @classmethod
    def launch_instance(cls, argv=None):
        pre_init_options, new_argv = cls.parse_pre_init_options(argv)
        asyncio.set_event_loop_policy(uvloop.EventLoopPolicy())

        patch_handlers()

        yappi.set_clock_type('wall')
        if pre_init_options.enable_profiling:
            yappi.start()
        super().launch_instance(argv=new_argv)

    async def init_spawners(self):
        # NB: we are fully masking JupyterHub.init_spawners method

        self.log.debug('Initializing spawners (patched JC)')
        db = self.db

        async def user_stopped(user, server_name):
            spawner = user.spawners[server_name]
            status = await spawner.poll()
            self.log.warning('User %s server stopped with exit code: %s', user.name, status)
            await self.proxy.delete_user(user, server_name)
            await user.stop(server_name)

        spawners_number = 0

        for orm_user in db.query(orm.User).options(selectinload(orm.User._orm_spawners)).all():
            user = self.users[orm_user]
            for name, orm_spawner in user.orm_spawners.items():
                if orm_spawner.server is None:
                    continue

                spawner = user.spawners[name]

                # idk how spawner can have no server when orm_spawner have
                if not spawner.server:
                    self.log.debug('%s not running', user.name)
                    continue

                self.log.info('pretend that %s still running', user.name)
                await SaltMinion(minion_id=spawner.server.ip, parent=self).safe_add_to_redis()
                spawner.add_poll_callback(user_stopped, user, name)
                spawner.start_polling()
                spawners_number += 1

        active_counts = self.users.count_active_users()

        TOTAL_USERS.set(len(self.users))
        RUNNING_SERVERS.set(active_counts['active'])

        return spawners_number

    async def start(self):
        self.write_unified_agent_config()
        await super().start()

main = JC.launch_instance
