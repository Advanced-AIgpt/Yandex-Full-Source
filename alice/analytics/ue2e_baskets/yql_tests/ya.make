PY2TEST()

OWNER(g:alice_analytics)

PEERDIR(
    yql/library/fastcheck/python
)

DATA(
    arcadia/alice/analytics/ue2e_baskets
)

TEST_SRCS(
    test_yql_syntax.py
)

END()
