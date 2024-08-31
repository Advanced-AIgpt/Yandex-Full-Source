PY2_PROGRAM()

OWNER(g:lipkin)

PEERDIR(
    jupytercloud/tools/lib
    contrib/python/attrs
    contrib/python/simplejson
)

PY_SRCS(
    MAIN main.py
)

END()
