"""Ensures user VMs are bound to their private domain"""

from datetime import timedelta

from traitlets import Instance, Integer, default

from jupytercloud.backend.dns.manager import DNSManager
from jupytercloud.backend.lib.clients.qyp import QYPClient
from jupytercloud.backend.services.base.app import JupyterCloudApp


class DNSSyncApp(JupyterCloudApp):
    jupyterhub_service_prefix = '/services/dns'
    manager = Instance(DNSManager)
    qyp = Instance(QYPClient)

    interval = Integer(15 * 60, config=True)

    port = Integer(8892, config=True)

    @default('qyp')
    def _qyp_default(self):
        return QYPClient.instance(parent=self)

    @default('manager')
    def _manager_default(self):
        return DNSManager.instance(parent=self)

    def start(self):
        super().start()

        self.io_loop.add_callback(self.sync_dns)

    async def _sync_dns(self):
        self.log.info('start dns sync')

        qyp_vms = await self.qyp.get_user_hosts()

        self.log.info('fetched %d existing vms from qyp', len(qyp_vms))

        await self.manager.sync_dns(qyp_vms)

    async def sync_dns(self):
        try:
            await self._sync_dns()
        except Exception:
            self.log.exception('error while sync_dns', exc_info=True)

        self.log.info('sleep for %d seconds', self.interval)

        self.io_loop.add_timeout(
            timedelta(seconds=self.interval),
            self.sync_dns,
        )


main = DNSSyncApp.launch_instance
