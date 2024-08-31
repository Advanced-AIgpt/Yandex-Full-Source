OWNER(g:alice_analytics)

PY3TEST()

PEERDIR(
    alice/analytics/ww/arabic/asr_annotation/aggregator/lib
)

TEST_SRCS(
    test_aggregation.py
)

NO_CHECK_IMPORTS()

END()
