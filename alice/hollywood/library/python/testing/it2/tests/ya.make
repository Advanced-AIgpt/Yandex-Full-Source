PY3TEST()

OWNER(
    g:hollywood
    vitvlkv
)

REQUIREMENTS(network:full)

PEERDIR(
    alice/hollywood/library/python/testing/it2
    alice/tests/library/service
    contrib/python/PyHamcrest
)

TEST_SRCS(
    tests.py
)

END()
