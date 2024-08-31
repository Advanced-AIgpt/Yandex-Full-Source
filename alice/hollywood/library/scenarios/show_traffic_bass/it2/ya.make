PY3TEST()

OWNER(
    yagafarov
    g:megamind
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(8)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/show_traffic_bass/proto
)

TEST_SRCS(
    tests.py
    tests_smart_display.py
)

DATA(
    # В этой директории будут храниться файлы реквестов и ответов источников (стабы)
    arcadia/alice/hollywood/library/scenarios/show_traffic_bass/it2/tests
    arcadia/alice/hollywood/library/scenarios/show_traffic_bass/it2/tests_smart_display
)

REQUIREMENTS(ram:10)

END()