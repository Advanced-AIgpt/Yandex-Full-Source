OWNER(g:alice_analytics)

PY3TEST()

PEERDIR(
    library/python/resource
    alice/analytics/operations/priemka/relevance
)

TEST_SRCS(
    test_relevance_config.py
)

END()
