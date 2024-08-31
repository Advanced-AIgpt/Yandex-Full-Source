OWNER(g:paskills)

PY3TEST()

TEST_SRCS(
    test_monitorado_config.py
)

PEERDIR(
    contrib/python/PyYAML
    library/python/testing/yatest_common
)

DEPENDS(
    alice/paskills/alerts
)

END()
