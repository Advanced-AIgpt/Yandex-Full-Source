PY3TEST()

OWNER(
    g:smarttv
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(1)
SET(HOLLYWOOD_SHARD video)
ENV(HOLLYWOOD_SHARD=video)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/protos/data/video
    alice/megamind/protos/scenarios
)

TEST_SRCS(
    tests.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/tv_home/it2/tests
)

REQUIREMENTS(ram:10)

END()
