import os
import logging

logger = logging.getLogger(__name__)

workers = int(os.environ.get('VINS_WORKERS_COUNT', 10))
worker_class = 'sync'

timeout = 300
PORT = os.environ.get('QLOUD_HTTP_PORT', '80')
bind = ['[::]:%s' % PORT]
preload_app = False
capture_output = True


def on_starting(server):
    os.environ.setdefault(
        "DJANGO_SETTINGS_MODULE",
        "vins_api.external_skill.settings"
    )
    # need import monotonic before os.fork
    import monotonic  # noqa
    pass

    from vins_api.external_skill.api import skill_resource
    preload_app = os.environ.get('VINS_PRELOAD_APP')
    if preload_app:
        logger.info('loading vins app: %s', preload_app)
        skill_resource.get_or_create_connected_app(preload_app)
        logger.info('loaded vins app: %s', preload_app)


def post_worker_init(worker):
    from vins_core.utils.updater import start_all_updaters

    # run separate threads in each fork
    if os.environ.get('VINS_ENABLE_BACKGROUND_UPDATES') == '1':
        start_all_updaters()
