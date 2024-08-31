PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/python/utils
    alice/library/python/testing/auth
    library/python/testing/yatest_lib
    library/python/vault_client
)

PY_SRCS(
    __init__.py
    hollywood.py
)

END()

RECURSE_FOR_TESTS(tests)
