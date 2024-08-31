PY3TEST()

OWNER(
    flimsywhimsy
    g:alice_quality
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/megamind/protos/common
    alice/memento/proto
    alice/protos/data
    contrib/python/PyHamcrest
)

TEST_SRCS(tests.py)

DATA(arcadia/alice/hollywood/library/scenarios/voice/it2/tests)

END()
