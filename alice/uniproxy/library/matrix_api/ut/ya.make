PY3TEST()

OWNER(
    g:matrix
)

TEST_SRCS(
    test_matrix_api.py
)

PEERDIR(
    alice/uniproxy/library/logging
    alice/uniproxy/library/matrix_api
    alice/uniproxy/library/testing

    contrib/python/tornado/tornado-4
)

END()
