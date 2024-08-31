PY3_PROGRAM()

OWNER(g:jupyter-cloud)

PEERDIR(
    jupytercloud/tools/lib
    contrib/python/pathlib2
    contrib/python/paramiko
)

PY_SRCS(
    MAIN main.py
)

END()
