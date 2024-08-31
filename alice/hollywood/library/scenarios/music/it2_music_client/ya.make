PY3TEST()

OWNER(
    vitvlkv
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

FORK_SUBTESTS()

SPLIT_FACTOR(8)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/scenarios/music/proto
    alice/library/restriction_level/protos
    alice/megamind/protos/scenarios
    contrib/python/PyHamcrest
)

PY_SRCS(music_sdk_helpers.py)

TEST_SRCS(
    conftest.py
    tests_legatus.py
    tests_music_client.py
    tests_navigator.py
    tests_search_app.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/music/it2_music_client/tests_music_client
    arcadia/alice/hollywood/library/scenarios/music/it2_music_client/tests_navigator
    arcadia/alice/hollywood/library/scenarios/music/it2_music_client/tests_search_app
)

REQUIREMENTS(ram:32)

END()
