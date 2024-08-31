PY3TEST()

OWNER(
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/news/proto
    alice/megamind/protos/scenarios
    contrib/python/PyHamcrest
)

TEST_SRCS(
    conftest.py
    tests_slots.py
    tests.py
)

DATA(arcadia/alice/hollywood/library/scenarios/news/it2/tests_slots)

REQUIREMENTS(ram:32)

END()
