from jupyterhub.handlers.static import CacheControlStaticFilesHandler

from jupytercloud.backend.static import get_static_path

from . import (
    arcadia, backup, events, jupyticket, metrics, misc, nb_template, nirvana, oauth, qyp, settings,
    startrek, vault, test,
)


default_handlers = []
for mod in (
    arcadia,
    backup,
    events,
    jupyticket,
    metrics,
    misc,
    nb_template,
    nirvana,
    oauth,
    qyp,
    settings,
    startrek,
    vault,
    test,
):
    default_handlers.extend(mod.default_handlers)


STATIC_PREFIX = '/jcstatic/'

default_handlers.extend([
    (STATIC_PREFIX + r'(.*)', CacheControlStaticFilesHandler, dict(path=get_static_path())),
])


def jc_static_url(path):
    return STATIC_PREFIX + path.lstrip('/')
