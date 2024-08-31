# coding: utf-8
from __future__ import unicode_literals

import logging
import inspect

from importlib import import_module


logger = logging.getLogger(__name__)


_apps = {}


def _create_app(app_id, app_conf, session_storage):
    common_params = dict(session_storage=session_storage, app_id=app_id, allow_wizard_request=True)

    # find app class
    if inspect.isclass(app_conf['class']):
        app_class = app_conf['class']
    elif isinstance(app_conf['class'], basestring):
        package, class_name = app_conf['class'].rsplit('.', 1)
        app_class = getattr(import_module(package), class_name)
    else:
        raise ValueError('Not loaded vins app class')

    intent_renames = app_conf.get('intent_renames')

    # find app configuration or pickled object
    if 'path' in app_conf:
        vins_app = app_class(vins_file=app_conf['path'], intent_renames=intent_renames, **common_params)
    elif 'app_config' in app_conf:
        if callable(app_conf['app_config']):
            vins_app = app_class(app_conf=app_conf['app_config'](), intent_renames=intent_renames, **common_params)
        else:
            vins_app = app_class(app_conf=app_conf['app_config'], intent_renames=intent_renames, **common_params)
    else:
        raise ValueError('Not found app_config or path in app_config: %s' % app_conf)

    return vins_app


def create_app(app_id, app_conf, session_storage):
    if app_id not in _apps:
        _apps[app_id] = _create_app(app_id, app_conf, session_storage)

    app = _apps[app_id]
    return app
