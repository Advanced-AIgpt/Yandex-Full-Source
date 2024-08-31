PY3_LIBRARY()

OWNER(
    nkodosov
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
)

PY_SRCS(
    stop_commands/alarm_stop.py
    stop_commands/music_player_stop.py
    stop_commands/navi_stop.py
    stop_commands/simple_stop.py
    stop_commands/thin_audio_player_stop.py
    stop_commands/timer_stop.py

    sound_commands/simple_sound.py
)

END()

RECURSE(
    generator
    runner
)
