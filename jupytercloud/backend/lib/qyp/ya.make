PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/traitlets
    contrib/python/more-itertools
    contrib/python/websockets

    jupytercloud/backend/lib/util
    jupytercloud/backend/lib/clients
)

PY_SRCS(
    __init__.py
    instance.py
    permissions.py
    quota.py
    special_instances.py
    status.py
    vm.py
)

END()
