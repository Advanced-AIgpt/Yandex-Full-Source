PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(SMALL)

TEST_SRCS(
    test_base_views.py
    test_views.py
)

PEERDIR(
    alice/paskills/penguinarium
    contrib/python/pytest-asyncio
)

END()
