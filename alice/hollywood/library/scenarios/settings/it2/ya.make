PY3TEST()

OWNER(
    lavv17
    g:alice_quality
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    tests_devices_settings.py
    tests_music_announce.py
)

REQUIREMENTS(ram:32)

END()
