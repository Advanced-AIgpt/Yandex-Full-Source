OWNER(g:alice_analytics)

PY3_LIBRARY()

PY_SRCS(
    utils.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
