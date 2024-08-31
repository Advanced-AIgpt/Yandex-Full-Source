"""Processes requests from IDM"""

from traitlets import Bool, Instance, Integer, Set, default

from jupytercloud.backend.lib.clients.tvm import TVMClient
from jupytercloud.backend.lib.db.configurable import JupyterCloudDB
from jupytercloud.backend.services.base.app import JupyterCloudApp

from .handlers import get_handlers


class IDMIntegrationApp(JupyterCloudApp):
    jupyterhub_service_prefix = '/services/idm'
    name = 'jupyter_idm_integration'

    port = Integer(8890, config=True)

    tvm_client_verify = Bool(True, config=True)
    tvm_whitelist = Set(Integer(), config=True)

    classes = [JupyterCloudDB]

    jupyter_cloud_db = Instance(JupyterCloudDB)
    tvm_client = Instance(TVMClient, config=True)

    @default('jupyter_cloud_db')
    def _jupyter_cloud_db_default(self):
        return JupyterCloudDB.instance(parent=self)

    @default('tvm_client')
    def _tvm_client_default(self):
        return TVMClient.instance(parent=self)

    def init_handlers(self):
        super().init_handlers()
        self.handlers.extend(get_handlers(self.jupyterhub_service_prefix))

    def init_tornado_settings(self):
        super().init_tornado_settings()
        self.tornado_settings.update({
            'db': self.jupyter_cloud_db,
            'tvm_client': self.tvm_client,
            'tvm_client_verify': self.tvm_client_verify,
            'tvm_whitelist': self.tvm_whitelist,
        })

    def init_app(self):
        self.jupyter_cloud_db.init_db()
        self.tvm_client.start()


main = IDMIntegrationApp.launch_instance
