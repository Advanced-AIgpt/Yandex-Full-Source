import logging
import os
import socket
from getpass import getuser

from jupyterhub.auth import DummyAuthenticator

import jupytercloud.backend.lib.util.config as uc
from jupytercloud.backend.handlers import default_handlers, jc_static_url
from jupytercloud.backend.lib.db.util import set_yc_connect_args
from jupytercloud.backend.lib.util.logging import log_request, setup_jupytercloud_logging
from jupytercloud.backend.spawner import QYPSpawner
from jupytercloud.backend.static import get_templates_path


# XXX: Единственное место, которое требует ручной правки
# хост, который будет использоваться для паспорта

USER = getuser()
PUBLIC_HOST = f'https://{USER}.jupyter.yandex-team.ru'

### Различные константы для переиспользования
DEV_SECRET_ID = 'sec-01dh6emwya97r6z2w8pc88m7a2'
TVM_DEV_SECRET_ID = 'sec-01e1h10grx4yq0fnv53czwa65q'
WORKDIR = uc.workdir()
SECRETS = uc.get_secrets(DEV_SECRET_ID)
SELF_TVM_ID = 2018688

c = get_config()
config_filename = os.path.abspath(__file__)
hub_ip = socket.gethostname()
hub_port = 8081
api_url = f'http://{hub_ip}:{hub_port}'


### Настройки самого JupyterHub

c.JupyterHub.admin_access = True
c.JupyterHub.cleanup_servers = False
c.JupyterHub.redirect_to_server = False

# This is the port of the proxy component, not of the hub itself
c.JupyterHub.port = 8000
# This is the IP of the hub component
c.JupyterHub.hub_ip = hub_ip
c.JupyterHub.hub_port = hub_port

c.JupyterHub.template_paths = [get_templates_path()]
c.JupyterHub.template_vars = {
    'documentation_link': 'asdasdasd',
    'chat_link': 'asdasd',
    'jc_static_url': jc_static_url,
    'version': uc.get_version(),
    'jc_debug': True,
    'jc_holiday': '🛠️',
    'jc_holiday_style': 'font-size:25px;padding-left:0px',
    'jc_holiday_title': 'Under development…'
}
c.JupyterHub.extra_handlers = default_handlers
c.JupyterHub.tornado_settings = {
    'slow_spawn_timeout': 1,
    'log_function': log_request,
    'jupyter_public_host': PUBLIC_HOST,
    'manual_backup_timeout': 60 * 60,
}

# local installation uses local dist by-default
# c.JC.webpack_assets_url = 'https://jupytercloud-static.s3.mds.yandex.net/webpack-assets-testing.json'

c.JC.external_js = [
    'https://yastatic.net/react/17.0.2/react-with-dom.js',
    'https://unpkg.com/react-bootstrap@2.0.0-beta.6/dist/react-bootstrap.min.js',
]

c.JupyterHub.logo_file = 'resfs/file/jupytercloud/backend/static/resources/logo.svg'

c.JupyterHub.services = [
    {
        'name': 'tvmtool',
        'command': ['tvmtool', '-e', '--config', uc.tvm_config(TVM_DEV_SECRET_ID, SELF_TVM_ID)],
        'environment': {'TVMTOOL_LOCAL_AUTHTOKEN': uc.tvm_local_token()},
        'admin': True,
        'cwd': str(WORKDIR),
    },
    # {
    #     "name": "idm",
    #     "url": "http://{}:8890".format(c.JupyterHub.hub_ip),
    #     "command": [str(uc.bin_path("jupytercloud_idm_service")), "--config", config_filename],
    #     "admin": True,
    #     "api_token": SECRETS['jupyterhub_cookie_secret'],
    #     "environment": os.environ.copy()
    # },
    # {
    #     'name': 'dns-sync',
    #     "url": "http://{}:8892".format(c.JupyterHub.hub_ip),
    #     'command': [str(uc.bin_path('jupytercloud_dns_sync')), '--config', config_filename],
    #     'environment': os.environ.copy(),
    # },
    # {
    #     'name': 'consistency-watcher',
    #     "url": "http://{}:8893".format(c.JupyterHub.hub_ip),
    #     'command': [str(uc.bin_path('consistency_watcher')), '--config', config_filename],
    #     'environment': os.environ.copy(),
    #     'admin': True,
    # }
]


### Настройки IDM

c.SpawnerForm.idm_url = (
    'https://idm.test.yandex-team.ru/system/jupyter/roles#rf=1,rf-role=HElsrO9D#user:{username}@jupyter/quota/cpu1_ram4_hdd24;;;,f-status=all,f-role=jupyter,sort-by=-updated,rf-expanded=HElsrO9D'
)
c.SpawnerForm.networks_doc_url = 'https://wiki.yandex-team.ru/jupyter/'
# c.IDMIntegrationApp.jupyterhub_api_url = api_url
# c.IDMIntegrationApp.jupyterhub_api_token = SECRETS['jupyterhub_cookie_secret']
# c.IDMIntegrationApp.tvm_client_verify = False
# c.IDMIntegrationApp.tvm_whitelist = {2018686, 2001600, 2001602}


### Настройки аутентификации, tvm, ouath

c.JupyterHub.authenticator_class = DummyAuthenticator

c.TVMClient.self_alias = 'jupytercloud_test'
c.TVMClient.auth_token = uc.tvm_local_token()
c.TVMClient.port = 18080

c.JupyterCloudOAuth.client_id = 'a5623a3259f146f7a81a35f71bd03ae2'
c.JupyterCloudOAuth.redirect_host = PUBLIC_HOST


### Настройки спавна

c.JupyterHub.spawner_class = QYPSpawner

c.QYPClient.oauth_token = c.YPClient.oauth_token = SECRETS['qyp_oauth_token']
c.QYPClient.clusters = {
    'sas': 'https://vmproxy.sas-swat.yandex-team.ru/',
    'vla': 'https://vmproxy.vla-swat.yandex-team.ru/',
    'iva': 'https://vmproxy.iva-swat.yandex-team.ru/',
    'myt': 'https://vmproxy.myt-swat.yandex-team.ru/',
}
c.QYPClient.vm_name_prefix = 'devel-jupyter-cloud-'
c.QYPClient.vm_short_name_prefix = 'djc-'

c.QYPSpawner.http_timeout = 10
c.QYPSpawner.progress_poll_interval = 2
c.QYPSpawner.stop_timeout = 60
c.QYPSpawner.start_internal_timeout = 3600
c.QYPSpawner.default_account_user = 'robot-jupyter-cloud'
c.QYPSpawner.default_account_id = 'abc:service:2142'
c.QYPSpawner.jupyter_cloud_environment = 'devel'
c.QYPSpawner.default_network = '_JUPYTER_CLOUD_TEST_NETS_'
c.QYPSpawner.network_whitelist = [
    '_JUPYTER_CLOUD_TEST_NETS_',
    '_JUPYTER_PARENT_NETS_',
    '_JUPYTER_CLOUD_OUTSTAFF_NETS_',
    '_JUPYTER_TAXI_NETS_',
]
c.QYPSpawner.disk_resource_filter = dict(
    resource_type='QEMU_IMAGE_JUPYTER_CLOUD_MINION',
    owner='JUPYTER_CLOUD',
    attrs=dict(
        environment='testing',
    ),
)

c.VMPermissions.jupyter_cloud_abc_group = '84258'
c.VMPermissions.jupyter_cloud_robot_user = 'robot-jupyter-cloud'

### Настройки salt

c.SaltClient.urls = [
    'http://salt-sas.beta.jupyter.yandex-team.ru',
    'http://salt-iva.beta.jupyter.yandex-team.ru',
    'http://salt-myt.beta.jupyter.yandex-team.ru',
]
c.SaltClient.username = PUBLIC_HOST
c.SaltClient.password = SECRETS['salt_secret']
c.SaltClient.eauth = 'sharedsecret'

c.SaltMinion.redis_data_bank = 'test/backend/minions'

JUPYTER_TEST_REDIS = [
    'http://man-e4n32zle4p444a1o.db.yandex.net:26379',
    'http://sas-zykvti0f3q1mml0x.db.yandex.net:26379',
    'http://vla-za5kqljlvd9cm1yd.db.yandex.net:26379',
]

c.RedisClient.password = SECRETS['redis_password']
c.RedisClient.sentinel_urls = JUPYTER_TEST_REDIS
c.RedisClient.sentinel_name = 'jupyter_test_redis'

### Настройки базы

db_cluster_id = '726876b0-fb04-49ca-80b0-0ffecdd7a687'
db_hosts = [
    'vla-zzhnnfzviapk32fc.db.yandex.net',
    'sas-dk55yd9fu0abm1ep.db.yandex.net',
]
db_user = 'robot_jupyter_cloud'
db_password = SECRETS['db_password']

set_yc_connect_args(
    c.JupyterCloudDB,
    cluster_id=db_cluster_id,
    hosts=db_hosts,
    user='robot_jupyter_cloud',
    password=db_password,
    port=6432,
    dbname='jupytercloud_test',
)


### Настройки различных клиентов вперемешку

c.SandboxClient.oauth_token = SECRETS['sandbox_oauth_token']

# c.JupyterHub.proxy_class = TraefikRedisProxy
# c.TraefikRedisProxy.should_start = False
# c.TraefikRedisProxy.traefik_api_username = 'jupyterhub'
# c.TraefikRedisProxy.traefik_api_password = SECRETS["traefik_password"]
# c.TraefikRedisProxy.kv_password = SECRETS["redis_password"]
#
# c.TraefikRedisProxy.sentinel_urls = JUPYTER_TEST_REDIS
# c.TraefikRedisProxy.sentinel_name = "jupyter_test_redis"
#
# c.TraefikRedisProxy.kv_traefik_prefix = "test/proxy/traefik/"
# c.TraefikRedisProxy.kv_jupyterhub_prefix = f"dev/{USER}/proxy/jupyterhub/"
# c.TraefikRedisProxy.databank = f"dev/{USER}/proxy/data"

c.ABCClient.oauth_token = SECRETS['abc_oauth_token']
c.ABCClient.url = 'https://abc-back.yandex-team.ru:8778/api/v4'
c.ABCClient.validate_cert = False

c.DNSApiClient.oauth_token = SECRETS['dns_api_oauth_token']
c.DNSApiClient.account = 'robot-jupyter-cloud'
c.DNSManager.user_zone = 'dev.jupyter.yandex-team.ru'

c.SVN.ssh_key_path = uc.id_rsa('sec-01d83hjj2yehgykzn85n2h5pa5')
c.SVN.ssh_user = 'robot-jupyter-cloud'

c.Arc.oauth_token = SECRETS['arc_token']

c.StaffClient.url = 'https://staff-api.yandex-team.ru:8784/v3'

c.JupyterClient.proxy_base_url = PUBLIC_HOST

c.StartrekClient.app_name = 'jupytercloud_lipkin'
c.StartrekClient.api_url = 'https://st-api.test.yandex-team.ru/v2'
c.StartrekClient.front_url = 'https://st.test.yandex-team.ru'

c.AsyncSSHClient.id_rsa = SECRETS['id_rsa']

### Логирование

setup_jupytercloud_logging(
    application_class_name='JC',
    log_dir=WORKDIR / 'log/jupyterhub',
    stdout_log_level=logging.INFO,
    asyncio_log_level=logging.ERROR,
)
setup_jupytercloud_logging(
    application_class_name='IDMIntegrationApp', log_dir=WORKDIR / 'log/idm',
)
setup_jupytercloud_logging(
    application_class_name='DNSSyncApp', log_dir=WORKDIR / 'log/dns-sync',
)
setup_jupytercloud_logging(
    application_class_name='ConsistencyWatcherApp', log_dir=WORKDIR / 'log/consistency-watcher',
)
# setup_sentry(
#     dsn=SECRETS['sentry_dsn'], environment_name='devel',
# )
