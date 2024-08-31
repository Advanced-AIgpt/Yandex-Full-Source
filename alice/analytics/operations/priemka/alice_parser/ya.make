OWNER(g:alice_analytics)

PY2_PROGRAM(alice_parser)

STRIP()

PEERDIR(
    alice/analytics/operations/priemka/alice_parser/lib
    alice/analytics/operations/priemka/alice_parser/utils
    alice/analytics/operations/priemka/alice_parser/visualize
    contrib/python/click
)

PY_SRCS(
    MAIN main.py
)

NO_CHECK_IMPORTS()

END()

RECURSE(
    udf
    utils
)

RECURSE_FOR_TESTS(
    tests
)

