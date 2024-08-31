PY3_PROGRAM(manage)

OWNER(g:jupyter-cloud)

PEERDIR(
    library/python/resource
    sandbox/common/rest
    contrib/python/click
    contrib/python/tqdm
    jupytercloud/arcadia_kernel/lib
)

PY_SRCS(
    MAIN manage.py
    install.py
    utils.py
    get_kernels.py
    archive.py
)

RESOURCE(jupytercloud/arcadia_kernel/package.json package.json)

END()
