PY3TEST()

OWNER(g:wonderlogs)

PEERDIR(
    contrib/python/pytest
    alice/wonderlogs/proto_tests/lib
)

TEST_SRCS(
    proto_checker.py
)

END()
