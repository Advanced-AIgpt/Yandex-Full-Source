SANDBOX_PY3_TASK()

OWNER(g:alice_fun)

PEERDIR(
    sandbox/common
    sandbox/projects/common/vcs
    alice/monitoring/alerts/solomon_syncer/lib
)

PY_SRCS(
    __init__.py
)

END()
