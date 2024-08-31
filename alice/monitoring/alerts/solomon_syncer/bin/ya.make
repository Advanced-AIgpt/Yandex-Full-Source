PY3_PROGRAM(solomon_syncer)

OWNER(g:alice_fun)

PEERDIR(
    contrib/python/click
    alice/monitoring/alerts/solomon_syncer/lib
)

PY_SRCS(
    __main__.py
)

END()
