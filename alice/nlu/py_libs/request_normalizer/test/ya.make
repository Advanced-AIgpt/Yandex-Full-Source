PY3TEST()

OWNER(
    g:alice_quality
)


PEERDIR(
    alice/nlu/py_libs/request_normalizer
    alice/nlu/libs/request_normalizer

    contrib/python/pytest
)

TEST_SRCS(
    test_request_normalizer.py
)

NO_CHECK_IMPORTS()

END()
