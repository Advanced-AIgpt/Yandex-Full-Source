PY3_PROGRAM(mock_launcher)

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/backend

    contrib/python/jupyterhub-traefik-proxy
    contrib/python/jupyterhub/jupyterhub/alembic
)

PY_SRCS(
    MAIN main.py
    backend/__init__.py
    backend/app.py
)

END()

