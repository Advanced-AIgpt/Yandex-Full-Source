PY3TEST()

TEST_SRCS(
     test.py
     test_parsers.py
)

PEERDIR(
    alice/beggins/cmd/manifestator/internal

    contrib/python/pytest
    contrib/python/pytest-mock
    contrib/python/requests-mock
)

END()
