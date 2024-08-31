PY3_LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/python/testing/run_request_generator
    alice/library/python/testing/megamind_request
    alice/library/python/testing/auth
)

PY_SRCS(
    srcrwr_base.py
    srcrwr_hardcoded.py
    srcrwr_thin.py
    tests_data_hardcoded_music.py
    tests_data_old_auto.py
    tests_data_user_no_plus.py
    tests_data_yabro.py
    tests_data.py
    tests_player_prev_next.py
    tests_player_shuffle.py
    tests_thin_client_no_audio_client.py
    tests_thin_client_no_plus.py
    tests_thin_client.py
    thin_client_common.py
)

END()

RECURSE(
    generator
    runner
)
