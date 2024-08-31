PY3TEST()

OWNER(
    sparkle
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

SPLIT_FACTOR(10)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/search/proto
    contrib/python/PyHamcrest
)

TEST_SRCS(
    test_open_apps_fixlist.py
    tests_search.py
    tests_search_centaur.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/search/it2/test_open_apps_fixlist
    arcadia/alice/hollywood/library/scenarios/search/it2/tests_search
)

REQUIREMENTS(ram:14)

END()
