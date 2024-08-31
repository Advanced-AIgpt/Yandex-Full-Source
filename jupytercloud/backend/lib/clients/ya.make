PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/async-lru
    contrib/python/asyncssh
    contrib/python/traitlets
    contrib/python/tenacity
    contrib/python/aioredis/aioredis-1
    contrib/python/paramiko
    contrib/python/asyncio-pool

    library/python/python-blackboxer
    library/python/tvmauth
    library/python/nirvana_api
    library/python/vault_client

    yp/python/client

    jupytercloud/backend/lib/util
)

PY_SRCS(
    __init__.py
    abc.py
    blackbox.py
    http.py
    jupyter.py
    jupyterhub.py
    infra.py
    nirvana.py
    oauth.py
    qyp.py
    redis.py
    salt/__init__.py
    salt/minion.py
    sandbox.py
    startrek.py
    staff.py
    ssh.py
    tvm.py
    vault.py
    yp.py
)

END()
