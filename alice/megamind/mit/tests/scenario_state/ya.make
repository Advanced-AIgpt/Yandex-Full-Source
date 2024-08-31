PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

PEERDIR(
    alice/megamind/mit/tests/scenario_state/proto
    alice/megamind/protos/scenarios
)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

TEST_SRCS(
    scenario_state.py
)

END()

RECURSE(
    proto
)
