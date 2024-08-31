OWNER(
    g:paskills
)

PY3_PROGRAM()

PY_SRCS(
    __main__.py
)

PEERDIR(
    library/python/monitoring/solo/helpers
    alice/paskills/alerts/solomon_alerts/registry
)

END()
