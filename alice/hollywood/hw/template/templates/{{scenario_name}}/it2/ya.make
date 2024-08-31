PY3TEST()

OWNER(
    {{username}}
    g:hollywood
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

REQUIREMENTS(ram:32)

PEERDIR(
    alice/hollywood/library/scenarios/{{scenario_name}}/proto
)

DATA(arcadia/alice/hollywood/library/scenarios/{{scenario_name}}/it2/tests)

END()
