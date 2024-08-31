PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
)

PEERDIR(
    alice/tests/library/region
    alice/tests/library/vault
)

END()

RECURSE_FOR_TESTS(tests)
