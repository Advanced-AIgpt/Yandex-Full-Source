import logging
import os

import jupytercloud.backend.lib.util.config as uc
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging


DEV_SECRET_ID = 'sec-01dh6emwya97r6z2w8pc88m7a2'
TVM_DEV_SECRET_ID = 'sec-01e1h10grx4yq0fnv53czwa65q'
WORKDIR = uc.workdir()
SECRETS = uc.get_secrets(DEV_SECRET_ID)
SELF_TVM_ID = 2018688

c = get_config()

yenv = os.getenv('YENV_TYPE')
assert yenv

c.RedisClient.sentinel_urls = [
    'http://man-e4n32zle4p444a1o.db.yandex.net:26379',
    'http://sas-zykvti0f3q1mml0x.db.yandex.net:26379',
    'http://vla-za5kqljlvd9cm1yd.db.yandex.net:26379',
]
c.RedisClient.sentinel_name = 'jupyter_test_redis'
c.RedisClient.password = SECRETS['redis_password']
c.SaltClient.username = 'key_accepter'
c.SaltClient.eauth = 'sharedsecret'

c.KeyAccepterApp.port = 9999
c.KeyAccepterApp.key_directory = WORKDIR / 'keys'

if yenv == 'development':
    c.SaltClient.urls = ['http://salt-sas.beta.jupyter.yandex-team.ru', 'http://salt-iva.beta.jupyter.yandex-team.ru']
    c.KeyAccepterApp.minion_redis_bank = 'test/backend/minions'
    c.SaltMinion.redis_data_bank = 'test/backend/minions'
    c.SaltClient.password = SECRETS['salt_secret']

setup_jupytercloud_logging(
    'KeyAccepterApp',
    log_dir='local-key-accepter',
    stdout_log_level=logging.DEBUG,
)
