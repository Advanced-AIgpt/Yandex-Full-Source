OWNER(g:alice_analytics)

PY23_LIBRARY()

PY_SRCS(
    metrics_viewer.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
