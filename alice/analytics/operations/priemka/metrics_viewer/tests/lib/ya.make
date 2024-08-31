OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    alice/analytics/operations/priemka/metrics_viewer
)

TEST_SRCS(
    test_metrics_viewer.py
)

END()
