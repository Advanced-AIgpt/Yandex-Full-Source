PY3TEST()

OWNER(g:alice)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

PEERDIR(
    alice/bass/fixture
    contrib/python/pytest
)

END()
