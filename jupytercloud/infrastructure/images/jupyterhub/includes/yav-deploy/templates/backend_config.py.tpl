import os
import logging
import re

from jupyterhub_traefik_proxy import TraefikRedisProxy

from jupytercloud.backend.auth import YandexBlackboxAuthenticator
from jupytercloud.backend.lib.clients.infra import InfraFilter
from jupytercloud.backend.lib.db.util import set_yc_connect_args
from jupytercloud.backend.handlers import default_handlers as extra_handlers, jc_static_url
from jupytercloud.backend.spawner import QYPSpawner
from jupytercloud.backend.static import get_templates_path
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging, log_request
from jupytercloud.backend.lib.util.sentry import setup_sentry

from library.python.svn_version import commit_id, svn_branch

c = get_config()

config_filename = os.path.abspath(__file__)

yenv = os.getenv('YENV_TYPE')
assert yenv

c.JupyterHub.admin_access = True
c.JupyterHub.cleanup_servers = False
c.JupyterHub.cleanup_proxy = False
c.JupyterHub.authenticate_prometheus = False

# this internal url will be used for proxy redirects
c.JupyterHub.hub_ip = ''
c.JupyterHub.hub_bind_url = 'https://[::]:8081'
c.JupyterHub.hub_port = 8081
c.JupyterHub.port = 8081
# bytes string is required
c.JupyterHub.cookie_secret = b"{{ jupyterhub_cookie_secret }}"

# after this timeout JH will redirect to a page with spawn progressbar;
# we decrease timeout from default 10 seconds
c.JupyterHub.tornado_settings = {
    'slow_spawn_timeout': 1,
    'log_function': log_request,
    'gzip': True
}
c.JupyterHub.logo_file = 'resfs/file/jupytercloud/backend/static/resources/logo.svg'

c.JupyterHub.redirect_to_server = False

c.JupyterHub.spawner_class = QYPSpawner
c.QYPSpawner.http_timeout = 120
c.QYPSpawner.stop_timeout = 600
c.QYPSpawner.default_account_user = 'robot-jupyter-cloud'
c.QYPSpawner.default_account_id = 'abc:service:2142'
c.QYPSpawner.poll_interval = 300

c.VMPermissions.jupyter_cloud_abc_group = '84258'
c.VMPermissions.jupyter_cloud_robot_user = 'robot-jupyter-cloud'

c.SpawnerForm.networks_doc_url = 'https://wiki.yandex-team.ru/jupyter/docs/customnetwork/'

c.JupyterHub.proxy_class = TraefikRedisProxy
c.TraefikRedisProxy.should_start = False
c.TraefikRedisProxy.traefik_api_username = 'jupyterhub'
c.TraefikRedisProxy.traefik_api_password = "{{ traefik_password }}"
c.TraefikRedisProxy.kv_password = "{{redis_password}}"

JUPYTER_TEST_REDIS = [
    "http://sas-zykvti0f3q1mml0x.db.yandex.net:26379",
    "http://vla-za5kqljlvd9cm1yd.db.yandex.net:26379"
]

c.RedisClient.password = "{{redis_password}}"
c.RedisClient.sentinel_urls = JUPYTER_TEST_REDIS
c.RedisClient.sentinel_name = "jupyter_test_redis"

c.TVMClient.self_alias = "jupytercloud"
c.TVMClient.port = 2

c.BackendSwitcherApp.service_name = 'jupyter_test_redis'
c.BackendSwitcherApp.sentinels = JUPYTER_TEST_REDIS
c.BackendSwitcherApp.redis_password = '{{ redis_password }}'

c.Pagination.default_per_page = 2000
c.Pagination.max_per_page = 5000

if yenv == 'development':
    PUBLIC_URL = 'https://beta.jupyter.yandex-team.ru'

    db_cluster_id = '726876b0-fb04-49ca-80b0-0ffecdd7a687'
    db_hosts = [
        'vla-zzhnnfzviapk32fc.db.yandex.net',
        'sas-dk55yd9fu0abm1ep.db.yandex.net',
    ]
    db_name_jc = 'jupytercloud_test'
    db_name_jh = 'jupyterhub_test'
    db_log_level = logging.DEBUG

    c.QYPSpawner.jupyter_cloud_environment = 'testing'
    c.QYPClient.vm_name_prefix = 'testing-jupyter-cloud-'
    c.QYPClient.vm_short_name_prefix = 'tjc-'
    c.QYPSpawner.default_network = '_JUPYTER_CLOUD_TEST_NETS_'
    c.QYPSpawner.network_whitelist = [
        '_JUPYTER_CLOUD_TEST_NETS_',
        '_JUPYTER_PARENT_NETS_',
        '_JUPYTER_TAXI_NETS_',
        '_BI_YP_UNSTABLE_NETS_',
        '_JUPYTER_SHMYA_NETS_',
        '_STRM_DEV_NETS_',
        '_JUPYTER_MEDIAANALYST_OUTSTAFF_NETS_',
        '_TRAVEL_ANALYTICS_OUTSTAFF_NETS_',
    ]

    c.QYPSpawner.disk_resource_filter = dict(
        resource_type="QEMU_IMAGE_JUPYTER_CLOUD_MINION",
        attrs=dict(
            environment="testing",
        )
    )

    c.TVMClient.self_id = 2018688

    SALT_URLS = [
        'http://salt-sas.beta.jupyter.yandex-team.ru',
        'http://salt-iva.beta.jupyter.yandex-team.ru',
        'http://salt-myt.beta.jupyter.yandex-team.ru'
    ]
    c.SaltClient.urls = SALT_URLS

    c.SaltMinion.redis_data_bank = "test/backend/minions"

    c.StaffClient.url = 'https://staff-api.test.yandex-team.ru/v3'

    c.SpawnerForm.idm_url = "https://idm.test.yandex-team.ru/#rf-role=HElsrO9D#user:{username}@jupyter/quota/cpu1_ram4_hdd24;;;,rf-expanded=HElsrO9D,rf=1"

    c.JupyterCloudOAuth.client_id = "0bdc337f290748b982b1e8bc0c345cca"

    chat_url = "https://nda.ya.ru/3Vmw7U"
    deploy_link = "https://deploy.yandex-team.ru/stages/jupytercloud-hub-test"

    c.EventsConfigurable.infra_filters = [
        InfraFilter(service_id=931, environment_id=1300, duration=14),  # JupyterCloud/Testing
    ]

    c.TraefikRedisProxy.sentinel_urls = JUPYTER_TEST_REDIS
    c.TraefikRedisProxy.sentinel_name = "jupyter_test_redis"

    c.TraefikRedisProxy.kv_traefik_prefix = "test/proxy/traefik/"
    c.TraefikRedisProxy.kv_jupyterhub_prefix = "test/proxy/jupyterhub/"
    c.TraefikRedisProxy.databank = "test/proxy/data"

    c.BackendSwitcherApp.redis_key = "test/backend/semaphore"

    c.StartrekClient.app_name = 'jupytercloud_testing'
    c.StartrekClient.api_url = 'https://st-api.test.yandex-team.ru/v2'
    c.StartrekClient.front_url = 'https://st.test.yandex-team.ru'

    c.JC.webpack_assets_url = "https://jupytercloud-static.s3.mds.yandex.net/webpack-assets-testing.json"
    c.JC.external_js = [
        "https://yastatic.net/react/17.0.2/react-with-dom.js",
        "https://unpkg.com/react-bootstrap@2.0.0-beta.6/dist/react-bootstrap.min.js",
    ]

elif yenv == 'production':
    PUBLIC_URL = 'https://jupyter.yandex-team.ru'

    db_cluster_id = 'e8515101-dec2-4b85-9c91-5e248bd3b161'
    db_hosts = [
        'sas-p3kj2q6yocz7xaua.db.yandex.net',
        'vla-50rd1dr37j1gdjsf.db.yandex.net',
    ]
    db_name_jc = 'jupytercloud'
    db_name_jh = 'jupyterhub'
    db_log_level = logging.WARNING

    c.QYPSpawner.jupyter_cloud_environment = 'production'
    c.QYPClient.vm_name_prefix = 'jupyter-cloud-'
    c.QYPClient.vm_short_name_prefix = 'jc-'
    c.QYPSpawner.default_network = '_JUPYTER_CLOUD_PROD_NETS_'
    c.QYPSpawner.network_whitelist = [
        '_JUPYTER_CLOUD_PROD_NETS_',
        '_JUPYTER_TAXI_NETS_',
        '_BI_YP_UNSTABLE_NETS_',
        '_JUPYTER_SHMYA_NETS_',
        '_JUPYTER_SCHOOLBOOK_EXTERNAL_NETS_',
        '_JUPYTER_PRAKTIKUM_EXTERNAL_NETS_',
        '_STRM_DEV_NETS_',
        '_TRAVEL_ANALYTICS_OUTSTAFF_NETS_',
    ]

    c.QYPSpawner.disk_resource_filter = dict(
        resource_type="QEMU_IMAGE_JUPYTER_CLOUD_MINION",
        attrs=dict(
            environment="production",
        )
    )

    c.TVMClient.self_id = 2018686

    SALT_URLS = [
        'http://salt-sas.jupyter.yandex-team.ru',
        'http://salt-iva.jupyter.yandex-team.ru',
        'http://salt-myt.jupyter.yandex-team.ru'
    ]
    c.SaltClient.urls = SALT_URLS

    c.SaltMinion.redis_data_bank = "prod/backend/minions"

    c.StaffClient.url = 'https://staff-api.yandex-team.ru/v3'

    c.SpawnerForm.idm_url = "https://idm.yandex-team.ru/#rf-role=1W1ve9BZ#user:{username}@jupyter/quota/cpu1_ram4_hdd24;;;,rf-expanded=1W1ve9BZ,rf=1"

    c.JupyterCloudOAuth.client_id = "82ede8f30a9347379538bc0877730928"

    chat_url = "https://nda.ya.ru/3UYxBK"
    deploy_link = "https://deploy.yandex-team.ru/stages/jupytercloud-hub-prod"

    c.TraefikRedisProxy.sentinel_urls = JUPYTER_TEST_REDIS
    c.TraefikRedisProxy.sentinel_name = "jupyter_test_redis"

    c.TraefikRedisProxy.kv_traefik_prefix = "prod/proxy/traefik/"
    c.TraefikRedisProxy.kv_jupyterhub_prefix = "prod/proxy/jupyterhub/"
    c.TraefikRedisProxy.databank = "prod/proxy/data"

    c.BackendSwitcherApp.redis_key = "prod/backend/semaphore"

    c.StartrekClient.app_name = 'jupytercloud'
    c.StartrekClient.api_url = 'https://st-api.yandex-team.ru/v2'
    c.StartrekClient.front_url = 'https://st.yandex-team.ru'

    c.JC.webpack_assets_url = 'https://jupytercloud-static.s3.mds.yandex.net/webpack-assets-production.json'
    c.JC.external_js = [
        "https://yastatic.net/react/17.0.2/react-with-dom.min.js",
        "https://unpkg.com/react-bootstrap@2.0.0-beta.6/dist/react-bootstrap.min.js",
    ]

c.TraefikRedisProxy.traefik_api_url = f"{PUBLIC_URL}/services/proxy"
c.JupyterClient.proxy_base_url = PUBLIC_URL
c.JupyterHub.tornado_settings['jupyter_public_host'] = PUBLIC_URL
c.JupyterHub.last_activity_interval = 6 * 3600
c.JupyterHub.service_check_interval = 300


db_user = 'robot_jupyter_cloud'
db_port = '6432'
db_password = '{{ db_password }}'

set_yc_connect_args(
    c.JupyterCloudDB,
    cluster_id=db_cluster_id,
    hosts=db_hosts,
    user='robot_jupyter_cloud',
    password='{{ db_password }}',
    port=6432,
    dbname=db_name_jc,
)

set_yc_connect_args(
    c.JupyterHub,
    cluster_id=db_cluster_id,
    hosts=db_hosts,
    user='robot_jupyter_cloud',
    password='{{ db_password }}',
    port=6432,
    dbname=db_name_jh,
)

c.JupyterHub.hub_connect_url = PUBLIC_URL

c.SaltClient.username = PUBLIC_URL
c.SaltClient.password = '{{ salt_secret }}'  # there are " symbol in dev salt secret
c.SaltClient.eauth = 'sharedsecret'

c.QYPClient.oauth_token = c.YPClient.oauth_token = '{{ qyp_oauth_token }}'
c.QYPClient.clusters = {
    'sas': 'https://vmproxy.sas-swat.yandex-team.ru/',
    'vla': 'https://vmproxy.vla-swat.yandex-team.ru/',
    'iva': 'https://vmproxy.iva-swat.yandex-team.ru/',
    'myt': 'https://vmproxy.myt-swat.yandex-team.ru/',
}

c.JupyterHub.authenticator_class = YandexBlackboxAuthenticator
c.YandexBlackboxAuthenticator.host = PUBLIC_URL

c.SaltKeyAccepter.port = 8891
c.DNSSyncApp.port = 8892
c.ConsistencyWatcherApp.port = 8893
c.BackendSwitcherApp.port = 8894
c.KeyAccepterApp.port = 8895

PRIVATE_URL = os.getenv("DEPLOY_POD_PERSISTENT_FQDN")
c.JupyterHub.services = [
    {
        'name': 'idm',
        'admin': True,
        'api_token': '{{ idm_api_token }}',
        'url': PUBLIC_URL
    },
    {
        'name': 'proxy-sync',
        'admin': True,
        'api_token': '{{ proxy_api_token }}',
    },
    {
        'name': 'sandbox',
        'admin': True,
        'api_token': '{{ sandbox_api_token }}',
    },
    {
        'name': 'dns',
        'command': ['/srv/jupytercloud', 'dns-sync', '--config', config_filename],
        'environment': {
            'YENV_TYPE': os.getenv('YENV_TYPE'),
        },
        'url': f'http://{PRIVATE_URL}:{c.DNSSyncApp.port}',
    },
    {
        'name': 'consistency-watcher',
        'command': ['/srv/jupytercloud', 'consistency-watcher', '--config', config_filename],
        'admin': True,
        'environment': os.environ.copy(),
        'url': f'http://{PRIVATE_URL}:{c.ConsistencyWatcherApp.port}',
    },
    {
        'name': 'backend-switcher',
        'url': f'http://{PRIVATE_URL}:{c.BackendSwitcherApp.port}',
    }
]

# TODO(JUPYTER-600): find out why these requests break our Tornado
# c.JupyterHub.services.extend(({
#     'name': re.search(r"//([^.]+)", salt_master)[1] + '-key-accepter',
#     'url': f'{salt_master}:{c.KeyAccepterApp.port}'
# } for salt_master in SALT_URLS))

if yenv == 'development':
    setup_sentry(
        dsn="https://c6aae3587aca45c997d61bd3740ee87f@sentry.stat.yandex-team.ru/468",
        environment_name=yenv
    )

setup_jupytercloud_logging(
    'JC',
    log_dir='/var/log/jupyterhub',
    db_log_level=db_log_level,
)
setup_jupytercloud_logging(
    'SaltKeyAccepter',
    log_dir='/var/log/salt-key-accepter'
)
setup_jupytercloud_logging(
    'DNSSyncApp',
    log_dir='/var/log/jupytercloud-dns-sync'
)
setup_jupytercloud_logging(
    'ConsistencyWatcherApp',
    log_dir='/var/log/consistency-watcher'
)
setup_jupytercloud_logging(
    'BackendSwitcherApp',
    log_dir='/var/log/backend-switcher'
)

c.JupyterHub.template_paths = [get_templates_path()]
c.JupyterHub.template_vars = {
    'documentation_link': 'https://wiki.yandex-team.ru/jupyter/docs/',
    'chat_link': chat_url,
    'deploy_link': deploy_link,
    'jc_static_url': jc_static_url,
    'dc': os.getenv("DEPLOY_NODE_DC", "🤷‍"),
    'version': f'{commit_id()}.{svn_branch()[:16]}',
    'jc_debug': yenv == 'development',
}

c.JupyterHub.extra_handlers = extra_handlers

c.SandboxClient.oauth_token = "{{ sandbox_oauth_token }}"
c.ABCClient.oauth_token = "{{ abc_oauth_token }}"  # TODO:
c.InfraClient.oauth_token = "{{ abc_oauth_token }}"  #  Rename secret key to jupytercloud-internal-oauth
c.EventsConfigurable.migration_doc_url = "https://nda.ya.ru/t/LNTJlwz93VxK8L"

c.SVN.ssh_key_path = '/srv/id_rsa'
c.SVN.ssh_user = 'robot-jupyter-cloud'
c.Arc.oauth_token = "{{ arc_token }}"

c.AsyncSSHClient.id_rsa = """{{ id_rsa }}"""

### DNS

c.DNSApiClient.oauth_token = "{{ dns_api_oauth_token }}"
c.DNSApiClient.account = "robot-jupyter-cloud"

if yenv == 'development':
    c.DNSManager.user_zone = 'test.jupyter.yandex-team.ru'
elif yenv == 'production':
    c.DNSManager.user_zone = 'user.jupyter.yandex-team.ru'
