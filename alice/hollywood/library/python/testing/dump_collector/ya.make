PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/library/python/eventlog_wrapper
)

PY_SRCS(
    __init__.py
    dump_collector.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
