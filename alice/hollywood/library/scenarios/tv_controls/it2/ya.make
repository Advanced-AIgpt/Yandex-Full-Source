PY3TEST()

OWNER(
    dandex
    g:smarttv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

PEERDIR(
    alice/hollywood/library/scenarios/tv_controls/proto
)

DATA(arcadia/alice/hollywood/library/scenarios/tv_controls/it2/tests)

END()
