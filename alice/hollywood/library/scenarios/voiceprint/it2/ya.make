PY3TEST()

OWNER(
    klim-roma
    g:hollywood
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

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/voiceprint/proto
    alice/megamind/protos/scenarios
    alice/protos/endpoint/capabilities/bio
)

PY_SRCS(voiceprint_helpers.py)

TEST_SRCS(
    conftest.py
    tests_set_my_name.py
    tests_voiceprint_enroll.py
    tests_voiceprint_remove.py
    tests_what_is_my_name.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/voiceprint/it2/passport_data
    arcadia/alice/hollywood/library/scenarios/voiceprint/it2/tests_set_my_name
    arcadia/alice/hollywood/library/scenarios/voiceprint/it2/tests_voiceprint_enroll
    arcadia/alice/hollywood/library/scenarios/voiceprint/it2/tests_voiceprint_remove
    arcadia/alice/hollywood/library/scenarios/voiceprint/it2/tests_what_is_my_name
)

END()

