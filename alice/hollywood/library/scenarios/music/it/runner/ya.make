PY3TEST()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

SIZE(MEDIUM)

FORK_SUBTESTS()

SPLIT_FACTOR(8)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/runner_common.inc)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/scenarios/music/it
    alice/hollywood/library/scenarios/music/proto
)

TEST_SRCS(
    tests_hardcoded_music.py
    tests_old_auto.py
    tests_player_next_prev.py
    tests_player_shuffle.py
    tests_thin_client_no_audio_client.py
    tests_thin_client_no_plus.py
    tests_thin_client.py
    tests_user_no_plus.py
    tests_yabro.py
    tests.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/music/it/data
    arcadia/alice/hollywood/library/scenarios/music/it/data_auto_old
    arcadia/alice/hollywood/library/scenarios/music/it/data_hardcoded_music
    arcadia/alice/hollywood/library/scenarios/music/it/data_pp
    arcadia/alice/hollywood/library/scenarios/music/it/data_pp_unauthorized_user
    arcadia/alice/hollywood/library/scenarios/music/it/data_user_no_plus
    arcadia/alice/hollywood/library/scenarios/music/it/data_weekly_promo
    arcadia/alice/hollywood/library/scenarios/music/it/data_yabro
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client_no_audio_client
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client_no_plus
    arcadia/alice/hollywood/library/scenarios/music/it/data_player_prev_next
    arcadia/alice/hollywood/library/scenarios/music/it/data_player_shuffle
)

REQUIREMENTS(ram:32)

END()
