PY3TEST()

OWNER(
    lavv17
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/alice_show/proto
    alice/megamind/protos/common
    contrib/python/PyHamcrest
)

TEST_SRCS(tests.py)

DATA(arcadia/alice/hollywood/library/scenarios/alice_show/it2/tests)

END()
