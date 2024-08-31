# coding: utf-8

import logging
import multiprocessing
import os
from uuid import uuid4
from math import ceil
import rtlog

# need import monotonic before os.fork
import monotonic  # noqa

logger = logging.getLogger(__name__)
os.environ['VINS_LOAD_TF_ON_CALL'] = '1'

if 'VINS_WORKERS_COUNT' in os.environ:
    workers = int(os.environ['VINS_WORKERS_COUNT'])
else:
    if 'QLOUD_CPU_GUARANTEE' in os.environ:
        cpus = float(os.environ['QLOUD_CPU_GUARANTEE'])
    else:
        cpus = multiprocessing.cpu_count()

    # magic formula of workers count
    # cpu_usage = CPU time / wall time
    # response_time - 95 percentile of vins response in seconds

    cpu_usage = os.environ.get('VINS_CPU_USAGE', 0.288)
    response_time = os.environ.get('VINS_RESPONSE_TIME', 1.5)
    workers = int(ceil(cpus / cpu_usage * response_time))

    if 'QLOUD_MEMORY_LIMIT' in os.environ:
        workers_count_by_memory = int(os.environ['QLOUD_MEMORY_LIMIT']) / (1024**2) / 900
        workers = max(workers, workers_count_by_memory)

worker_class = 'sync'
timeout = 300
PORT = os.environ.get('QLOUD_HTTP_PORT', '80')
bind = ['[::]:%s' % PORT]
capture_output = True
backlog = 2 * workers

rtlog_file = os.environ.get('VINS_RTLOG_FILE', '')
if rtlog_file:
    rtlog.activate(rtlog_file, 'vins')


def on_starting(server):
    from vins_api.common.db import get_redis_connection, get_redis_knn_cache_connection
    from vins_api.common.knn_cache import setup_redis_knn_cache
    from vins_api.speechkit import api
    from vins_api.speechkit import settings
    from vins_core.utils.metrics import sensors, RedisMetricsStorage, FileMetricsStorage

    if os.environ.get('VINS_ENABLE_METRICS') == '1':
        sensors.setup(
            RedisMetricsStorage(
                get_redis_connection(),
                settings.VINS_METRICS_CONF,
            )
        )
    if os.environ.get('VINS_METRICS_FILE'):
        sensors.setup(
            FileMetricsStorage(logging.getLogger('metrics_logger'))
        )
    logging.config.dictConfig(settings.LOGGING)
    # uuid - push-client requires unique string in the first log line
    logger.info('start loading vins apps %s', str(uuid4()))
    if os.environ.get('VINS_DISABLE_REDIS_KNN_CACHE') != '1':
        setup_redis_knn_cache(get_redis_knn_cache_connection())
        logger.info('using redis knn cache')

    preload_app = os.environ.get('VINS_PRELOAD_APP', '')
    for app_id in preload_app.split(';'):
        app_id = app_id.strip()
        if app_id:
            logger.info('loading vins app: %s', app_id)
            resource_name = settings.CONNECTED_APPS.get(app_id, {}).get('resource', 'sk')
            vins_app = api.app_resources[resource_name].get_or_create_connected_app(app_id).vins_app
            for resource in api.app_resources.itervalues():
                resource.set_app(app_id, vins_app)
            logger.info('loaded vins app: %s', app_id)


def post_worker_init(worker):
    from vins_api.speechkit import settings
    from vins_api.speechkit import api
    from vins_core.dm.request import AppInfo, create_request
    from vins_core.utils.updater import start_all_updaters

    preload_app = os.environ.get('VINS_PRELOAD_APP', '')
    for app_id in preload_app.split(';'):
        app_id = app_id.strip()
        if app_id:
            logger.info('Initializing app %s', app_id)
            resource_name = settings.CONNECTED_APPS.get(app_id, {}).get('resource', 'sk')
            app = api.app_resources[resource_name].get_or_create_connected_app(app_id)
            app.vins_app.nlu.warm_up_taggers()

            # Warmup both knn models.
            # TODO(vl-trifonov) Remove this after AB (https://st.yandex-team.ru/DIALOG-7775)
            for knn_exp in ['brute_force_knn', 'hnsw_knn']:
                app_info = AppInfo(
                    app_id='com.yandex.vins',
                    app_version='0.0.1',
                    os_version='1',
                    platform='unknown',
                )
                location = {
                    'lon': 37.587937,
                    'lat': 55.733771
                }  # Yandex Office
                req_info = create_request(
                    str(uuid4()),
                    utterance=u'ты китик',
                    app_info=app_info,
                    location=location,
                    experiments={
                        "uniproxy_vins_sessions": "1",
                        knn_exp: "1",
                    }
                )

                app.handle_request(req_info)

    # run separate threads in each fork
    if os.environ.get('VINS_ENABLE_BACKGROUND_UPDATES') == '1':
        start_all_updaters()
