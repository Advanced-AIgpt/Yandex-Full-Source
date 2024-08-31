# coding: utf-8
import importlib
import logging

import attr
import yaml
from gevent import monkey

from .events import run_all, kill_all
from .state import get_state_by_name

logger = logging.getLogger(__name__)


@attr.s
class Context(object):
    state = attr.ib()
    config = attr.ib()


def _try_import(module_name):
    try:
        return importlib.import_module(module_name)
    except ImportError:
        base_package = __package__[:-len('ybot.core')]
        return importlib.import_module(base_package + module_name)


def setup(conf):
    core_opts = conf.pop('core', {})

    # logging
    level = core_opts.get('log_level', 'WARNING')
    logging.root.setLevel(level)

    # import modules
    for module_name in conf:
        if module_name == 'core':
            continue

        logger.debug('Loading module %s', module_name)
        _try_import(module_name)

    # state backend
    try:
        backend = core_opts.get('state_backend', 'ybot.core.state:inmemory')
        back_module, back_name = backend.split(':')
        _try_import(back_module)

        state_conf = core_opts.get('state_config', {})
        State = get_state_by_name(back_name)
        state = State(state_conf)
    except KeyError:
        logger.error("Can't import %s state backend", backend)
        raise

    return Context(state, conf)


def serve(config_fd):
    logging.basicConfig(
        level=logging.WARNING,
        format='%(asctime)s [%(levelname)s] [%(name)s] %(message)s'
    )

    monkey.patch_all()

    conf = yaml.safe_load(config_fd)
    ctx = setup(conf)

    try:
        run_all(ctx)
    except KeyboardInterrupt:
        logger.info('Exit')
    except Exception:
        logger.exception('Unhandled exception, exit')
    finally:
        kill_all()
