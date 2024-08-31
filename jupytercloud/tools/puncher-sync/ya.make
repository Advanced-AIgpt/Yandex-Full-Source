PY3_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/ruamel.yaml
    contrib/python/requests
    library/python/oauth
    jupytercloud/tools/lib
)

PY_SRCS(
    MAIN main.py
)

END()
