PY3TEST()

OWNER(
    nkodosov
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(8)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/fast_command/it
)

TEST_SRCS(
    stop_commands/run_alarm_stop.py
    stop_commands/run_music_player_stop.py
    stop_commands/run_navi_stop.py
    stop_commands/run_simple_stop.py
    stop_commands/run_thin_audio_player_stop.py
    stop_commands/run_timer_stop.py

    sound_commands/run_simple_sound.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/fast_command/it/data
)

REQUIREMENTS(ram:32)

END()
