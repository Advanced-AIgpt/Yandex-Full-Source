PY3TEST()

OWNER(zubchick)

TEST_SRCS(
     test.py
)

PEERDIR(
    contrib/python/attrs
    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/requests-mock

    alice/review_bot/lib
)

SIZE(SMALL)

END()
