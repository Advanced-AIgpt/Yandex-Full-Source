PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/python/utils
)

PY_SRCS(
    __init__.py
    megamind.py
)

END()

RECURSE_FOR_TESTS(tests)
