import os
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging

c = get_config()

yenv = os.getenv('YENV_TYPE')
assert yenv

c.RedisClient.sentinel_urls = [
    'http://sas-zykvti0f3q1mml0x.db.yandex.net:26379',
    'http://vla-za5kqljlvd9cm1yd.db.yandex.net:26379'
]
c.RedisClient.sentinel_name = 'jupyter_test_redis'
c.RedisClient.password = '{{ redis_password }}'

c.SaltClient.urls = ['http://localhost:80']
c.SaltClient.username = 'key_accepter'
c.SaltClient.password = '{{ salt_secret }}'
c.SaltClient.eauth = 'sharedsecret'

dc = os.getenv('DEPLOY_NODE_DC')
c.KeyAccepterApp.jupyterhub_service_prefix = f'/services/salt-{dc}-key-accepter'
c.KeyAccepterApp.port = 8891

if yenv == 'development':
    c.SaltMinion.redis_data_bank = 'test/backend/minions'

elif yenv == 'production':
    c.SaltMinion.redis_data_bank = 'prod/backend/minions'

setup_jupytercloud_logging(
    'KeyAccepterApp',
    log_dir='/var/log/local-key-accepter',
)
