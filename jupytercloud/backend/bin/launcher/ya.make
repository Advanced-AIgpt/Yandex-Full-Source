PY3_PROGRAM(jupytercloud)

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/backend

    contrib/python/jupyterhub/jupyterhub/alembic
    contrib/python/psycopg2
    jupytercloud/contrib/jupyterhub-traefik-proxy

)

PY_SRCS(
    MAIN main.py
)

END()

