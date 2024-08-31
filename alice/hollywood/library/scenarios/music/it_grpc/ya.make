PY3TEST()

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

SPLIT_FACTOR(10)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it_grpc/common.inc)

PEERDIR(
    alice/cachalot/api/protos
    alice/hollywood/library/python/testing/it2
    alice/hollywood/library/scenarios/music/proto
    apphost/lib/grpc/protos
    apphost/lib/proto_answers
)

TEST_SRCS(
    common.py
    conftest.py
    tests_cache.py
    tests_centaur_collect_main_screen.py
)

DATA(arcadia/alice/hollywood/library/scenarios/music/it_grpc/data/)

REQUIREMENTS(ram:32)

END()
