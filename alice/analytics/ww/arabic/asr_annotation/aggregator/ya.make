OWNER(nerevar)

PY3_PROGRAM(ar_asr_annotation_aggregator)

PEERDIR(
    contrib/python/click
    alice/analytics/ww/arabic/asr_annotation/aggregator/lib
)

NO_CHECK_IMPORTS()

PY_SRCS(
    MAIN main.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
