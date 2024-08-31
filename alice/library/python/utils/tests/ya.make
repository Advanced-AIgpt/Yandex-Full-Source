PY3TEST()

OWNER(
    g:alice
)

PEERDIR(
    alice/library/python/utils
)

TEST_SRCS(
    test_network.py
)

END()
