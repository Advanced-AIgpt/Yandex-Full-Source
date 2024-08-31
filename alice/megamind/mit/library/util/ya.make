PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/library/python/eventlog_wrapper
    alice/megamind/mit/library/proto
    alice/tests/library/service
    library/python/testing/yatest_common
)

PY_SRCS(
    __init__.py
    util.py
)

END()
