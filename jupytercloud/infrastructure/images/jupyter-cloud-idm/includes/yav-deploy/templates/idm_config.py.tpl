import os
import logging

from jupytercloud.backend.lib.db.util import set_yc_connect_args
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging
from jupytercloud.backend.lib.util.sentry import setup_sentry

c = get_config()

yenv = os.getenv('YENV_TYPE')
assert yenv

if yenv == 'development':
    hub_url = 'https://beta.jupyter.yandex-team.ru'
    c.IDMIntegrationApp.log_level = 'DEBUG'
    c.IDMIntegrationApp.tvm_whitelist = {
        2001602,  # IDM testing
    }
    db_cluster_id = '726876b0-fb04-49ca-80b0-0ffecdd7a687'
    db_hosts = [
        'vla-zzhnnfzviapk32fc.db.yandex.net',
        'sas-dk55yd9fu0abm1ep.db.yandex.net',
    ]
    db_name = 'jupytercloud_test'

elif yenv == 'production':
    hub_url = 'https://jupyter.yandex-team.ru'
    c.IDMIntegrationApp.log_level = 'INFO'
    c.IDMIntegrationApp.tvm_whitelist = {
        2001600,  # IDM production
    }
    db_cluster_id = 'e8515101-dec2-4b85-9c91-5e248bd3b161'
    db_hosts = [
        'sas-p3kj2q6yocz7xaua.db.yandex.net',
        'vla-50rd1dr37j1gdjsf.db.yandex.net',
    ]
    db_name = 'jupytercloud'

db_log_level = logging.DEBUG

c.IDMIntegrationApp.jupyterhub_api_token = '{{ idm_api_token }}'
c.IDMIntegrationApp.jupyterhub_api_url = hub_url
c.IDMIntegrationApp.port = 8081

c.IDMIntegrationApp.tvm_client_verify = True
c.TVMClient.port = 2

set_yc_connect_args(
    c.JupyterCloudDB,
    cluster_id=db_cluster_id,
    hosts=db_hosts,
    user='robot_jupyter_cloud',
    password='{{ db_password }}',
    port=6432,
    dbname=db_name,
)

setup_jupytercloud_logging(
    'IDMIntegrationApp',
    log_dir='/var/log/jupyter-cloud-idm',
    db_log_level=db_log_level,
)

setup_sentry(
    dsn="https://c6aae3587aca45c997d61bd3740ee87f@sentry.stat.yandex-team.ru/468",
    environment_name=yenv
)
