PY3TEST()

OWNER(
    ran1s
    g:alice
)

INCLUDE(${ARCADIA_ROOT}/alice/acceptance/modules/check_analytics_info/tests/resources.inc)

PEERDIR(
    contrib/python/pytest
    library/python/resource
    alice/acceptance/modules/check_analytics_info/lib
)

TEST_SRCS(
     test_checkers.py
)

END()
