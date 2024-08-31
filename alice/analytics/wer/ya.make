PY3_LIBRARY()

OWNER(nstbezz)

PY_SRCS(
    metrics_counter.py
)

PEERDIR(
    contrib/python/pymorphy2
    contrib/python/pylev
)

END()

RECURSE_FOR_TESTS(
    tests
)
