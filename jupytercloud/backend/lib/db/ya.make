PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/jupyterhub
    contrib/python/sqlalchemy/sqlalchemy-1.3
    contrib/python/alembic
)

PY_SRCS(
    __init__.py
    configurable.py
    orm.py
    util.py
)

RESOURCE_FILES(
    PREFIX jupytercloud/backend/lib/db/
    alembic.ini
)

END()
