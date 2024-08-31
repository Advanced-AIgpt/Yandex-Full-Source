PY23_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/requests
)

PY_SRCS(
    TOP_LEVEL

    jupytercloud/__init__.py
    jupytercloud/library/__init__.py
    jupytercloud/library/yav.py
)

END()
