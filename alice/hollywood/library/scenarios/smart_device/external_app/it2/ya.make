PY3TEST()

OWNER(
    g:smarttv
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(1)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/megamind/protos/scenarios
)

TEST_SRCS(
    test_open_app.py
    test_lg_open_app.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/tv_home/it2/tests
)

REQUIREMENTS(ram:9)

END()
