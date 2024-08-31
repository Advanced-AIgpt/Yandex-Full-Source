PY3TEST()

OWNER(
    hellodima
    g:hollywood
)

SIZE(MEDIUM)

FORK_SUBTESTS()

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    test_clock.py
    test_do_not_disturb.py
    test_media_play.py
    test_media_session.py
    test_power_off.py
    test_sound.py
    test_start_multiroom.py
    test_stop.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/fast_command/it2/test_power_off
    arcadia/alice/hollywood/library/scenarios/fast_command/it2/test_start_multiroom
    arcadia/alice/hollywood/library/scenarios/fast_command/it2/test_stop
)

REQUIREMENTS(ram:32)

END()
