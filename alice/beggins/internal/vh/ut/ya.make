PY3TEST()

OWNER(zubchick)

TEST_SRCS(
     test_log_classifier.py
     test_find_threshold.py
)

PEERDIR(
    contrib/python/numpy
    contrib/python/scikit-learn
    alice/beggins/internal/vh/operations
    alice/beggins/internal/vh/scripts/python
)

DATA(sbr://2830218207)

SIZE(SMALL)

END()
