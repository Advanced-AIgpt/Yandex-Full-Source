PY3TEST()

OWNER(g:alice)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

PEERDIR(
    alice/hollywood/fixture
    contrib/python/pytest
)

END()
