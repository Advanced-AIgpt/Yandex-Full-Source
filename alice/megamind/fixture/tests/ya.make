PY3TEST()

OWNER(g:alice)

SIZE(SMALL)

TEST_SRCS(
    test.py
)

PEERDIR(
    alice/megamind/fixture
    contrib/python/pytest
)

END()
