PY2_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/pathlib2
    contrib/python/ujson
    jupytercloud/tools/lib
    statbox/nile
)

PY_SRCS(
    MAIN main.py
)

END()
