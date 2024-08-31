import re
import sys

from traitlets import List, Unicode, default

import jupytercloud.backend.lib.util.config as uc
from jupytercloud.backend.lib.qyp.status import VMStatus as QYPVMStatus
from jupytercloud.backend.services.consistency_watcher.app import ConsistencyWatcherApp, JHVMStatus


class MassSpawnApp(ConsistencyWatcherApp):
    config_file = Unicode('config.py', config=True)

    is_service = False

    jupyterhub_api_url = Unicode('https://jupyter.yandex-team.ru/hub/api', config=True)

    jupyterhub_api_token = Unicode(config=True)

    users = List(config=True)

    connected = set()
    failed = set()

    @default('jupyterhub_api_token')
    def _jupyterhub_api_token(self):
        secrets = uc.get_secrets('sec-01dhkemwckfe8tc5vbk1tps2yq')
        return secrets['sandbox_api_token']

    async def main_loop(self):
        if set(self.users) - self.connected - self.failed:
            await super().main_loop()
        else:
            self.log.info('no more users to spawn')
            self.log.info('successfully connected users: %s', sorted(self.connected))
            self.log.info('failed users: %s', sorted(self.failed))
            self.stop()

    async def load_qyp_users(self):
        users = {}

        for login in self.users:
            if vm := self.vms.get(login):
                if vm.status == JHVMStatus.connected:
                    self.connected.add(login)
                    continue

                if vm.failed_connections == self.connect_max_tries:
                    self.failed.add(login)
                    continue

            elif login in (self.failed | self.connected):
                continue

            users[login] = type(
                'VM', (), {
                    'login': login,
                    'is_evacuating': False,
                },
            )

        return users

    async def load_qyp_statuses(self, qyp_vms):
        return [QYPVMStatus.not_exists for _ in qyp_vms]


if __name__ == '__main__':
    sys.argv[0] = re.sub(r'(-script\.pyw?|\.exe)?$', '', sys.argv[0])
    sys.exit(MassSpawnApp.launch_instance())
