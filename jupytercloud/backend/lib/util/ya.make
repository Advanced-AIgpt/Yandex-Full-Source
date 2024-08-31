PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/python-slugify
    contrib/python/sentry-sdk
    contrib/python/yarl
    contrib/python/traitlets
    contrib/python/jupyterhub
    contrib/python/pycurl

    library/python/svn_version
    library/python/vault_client
    library/python/deploy_formatter
    library/python/monlib

    logbroker/unified_agent/client/python

    jupytercloud/backend/lib/db
)

PY_SRCS(
    metrics/__init__.py
    metrics/configurable.py
    metrics/tornado_client.py

    __init__.py
    config.py
    exc.py
    format.py
    logging.py
    misc.py
    paths.py
    report.py
    sentry.py
)

END()
