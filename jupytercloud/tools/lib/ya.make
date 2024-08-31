PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/requests
    contrib/python/coloredlogs
    contrib/python/paramiko
    contrib/python/tqdm
    contrib/python/psycopg2
    contrib/python/sentry-sdk
    contrib/python/salt-pepper
    contrib/python/tenacity
    library/python/vault_client

    jupytercloud/backend
)

PY_SRCS(
    __init__.py
    cloud.py
    parallel.py
    db.py
    environment.py
    utils.py
    jupyterhub.py
    report.py
)

END()
