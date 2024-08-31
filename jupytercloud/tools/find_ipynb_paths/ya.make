PY2_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/tools/lib
    contrib/python/click
)

PY_SRCS(
    MAIN main.py
)

END()
