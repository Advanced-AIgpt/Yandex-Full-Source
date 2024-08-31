import jupytercloud.backend.lib.util.config as uc
from jupytercloud.backend.lib.util.logging import setup_jupytercloud_logging


c = get_config()

WORKDIR = uc.workdir().parent

c.MassSpawnApp.jupyterhub_api_url = 'http://mjayyccw2p2iio4i.myt.yp-c.yandex.net:8081/hub/api'

c.MassSpawnApp.spawn_options = {
    'cluster': 'man',
    'account': 'abc:service:33985',
    'segment': 'dev',
    'preset': 'cpu6_ram24_ssd120',
    'quota_type': 'service',
    'network_id': '_JUPYTER_TAXI_NETS_',
    'setting-taxi_dmp_kernel': '1',
}

c.MassSpawnApp.sync_interval = 10
c.MassSpawnApp.connect_max_tries = 1
c.MassSpawnApp.log_level = 'INFO'

setup_jupytercloud_logging(
    application_class_name='MassSpawnApp',
    log_dir=WORKDIR / 'log',
    asyncio_log_level='ERROR',
)

c.MassSpawnApp.users = [
    'aabb',
    'aaraskin',
    'afgimadiev',
    'altsoph',
    'alytyakov',
    'ankozik',
    'anotkin',
    'anyamalkova',
    'apkonkova',
    'artemburnus',
    'daria-ko',
    'd-captain',
    'denchashch',
    'destitutiones',
    'dmastr',
    'dmitrybelyaev',
    'dryukalex',
    'eatroshkin',
    'evseev-sema',
    'galukhin',
    'gtbro',
    'hickinbottom',
    'igilyov',
    'igorrubanov',
    'ikars',
    'i-see-ya',
    'alex-sofronov',
    'assanskiy',
    'dgurev',
    'diskhakova',
    'dvgasparyan',
    'erik-nazarov',
    'gorbunov-yd',
    'ikhomyanin',
    'ishirokova19',
]
