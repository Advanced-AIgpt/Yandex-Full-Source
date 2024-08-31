PY3_PROGRAM(manifestator)

OWNER(
    alkapov
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    alice/beggins/cmd/manifestator/lib

    contrib/python/click

    yt/python/client
)

END()

RECURSE(
    internal
    lib
)

RECURSE_FOR_TESTS(
    tests
)
