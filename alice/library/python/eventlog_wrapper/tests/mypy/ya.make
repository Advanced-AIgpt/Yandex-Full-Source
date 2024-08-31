PY3TEST()

OWNER(
    g:hollywood
    g:megamind
    vitvlkv
    yagafarov
)

PEERDIR(
    alice/library/python/eventlog_wrapper
    
    library/python/testing/types_test/py3
)

TEST_SRCS(
    conftest.py
)

SIZE(MEDIUM)
TIMEOUT(600)

END()

