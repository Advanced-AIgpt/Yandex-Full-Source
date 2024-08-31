PY3TEST()

OWNER(
    g:hollywood
    g:megamind
    vitvlkv
    yagafarov
)

PEERDIR(
    alice/library/python/eventlog_wrapper
)

TEST_SRCS(
    test_wrapper.py
)

END()

RECURSE_FOR_TESTS(
    mypy
)
