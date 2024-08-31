PY3TEST()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

IF (CLANG_COVERAGE)
    # 1.5 hours timeout
    SIZE(LARGE)
    TAG(
        ya:fat
        ya:force_sandbox
        ya:sandbox_coverage
    )
ELSE()
    # 15 minutes timeout
    SIZE(MEDIUM)
ENDIF()

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(tests.py)

PEERDIR(
    alice/hollywood/library/scenarios/alarm/proto
)

DATA(arcadia/alice/hollywood/library/scenarios/alarm/it2/tests)

REQUIREMENTS(ram:14)

END()
