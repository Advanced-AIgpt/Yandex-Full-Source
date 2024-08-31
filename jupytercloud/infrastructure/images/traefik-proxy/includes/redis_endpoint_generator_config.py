import os
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging

c = get_config()

yenv = os.getenv('YENV_TYPE')
assert yenv

c.GenerateEndpointsApp.service_name = "jupyter_test_redis"
c.GenerateEndpointsApp.sentinels = [
    "sas-zykvti0f3q1mml0x.db.yandex.net",
    "vla-za5kqljlvd9cm1yd.db.yandex.net"
]
c.GenerateEndpointsApp.template_file = "/srv/traefik.toml.tpl"
c.GenerateEndpointsApp.endpoint_file = "/srv/traefik.toml"

setup_jupytercloud_logging(
    'GenerateEndpointsApp',
    log_dir='/var/log/endpoint_generator',
)
