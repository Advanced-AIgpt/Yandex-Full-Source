PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(SMALL)

TEST_SRCS(
    test_metrics.py
)

PEERDIR(
    alice/paskills/penguinarium
    contrib/python/fakeredis
    contrib/python/pytest-asyncio
)

END()
