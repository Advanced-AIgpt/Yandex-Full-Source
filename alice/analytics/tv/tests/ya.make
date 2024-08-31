OWNER(g:sda)

PY2TEST()

PY_SRCS(   
    tv_base.py
    base.py
    utils_tests.py
)

TEST_SRCS(
    test_dayuse.py
)

PEERDIR(
    contrib/python/fire
    statbox/nile
    statbox/qb2
    alice/analytics/utils
    alice/analytics/tv/cubes/dayuse
    alice/analytics/utils/relative_libs/relative_lib_common
)

DATA(
    arcadia/alice/analytics/tv/tests
)


FORK_TESTS()
SIZE(MEDIUM)

REQUIREMENTS(
    cpu:4
    ram:32
)


END()