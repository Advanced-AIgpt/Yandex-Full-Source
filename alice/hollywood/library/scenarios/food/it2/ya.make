PY3TEST()

OWNER(
    flimsywhimsy
    samoylovboris
    the0
    g:alice_quality
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/food/proto
    alice/megamind/protos/scenarios
)

TEST_SRCS(
    tests_food.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/food/it2/tests_food
)

REQUIREMENTS(ram:10)

END()
