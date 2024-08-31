PY2_PROGRAM()

OWNER(g:lipkin)

PEERDIR(
    jupytercloud/tools/lib
    sandbox/common
    infra/qyp/vmctl/src
)

PY_SRCS(
    MAIN main.py
)

END()
