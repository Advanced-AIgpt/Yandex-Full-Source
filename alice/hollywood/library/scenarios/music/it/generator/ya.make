PY3TEST()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/music/it
)

TEST_SRCS(
    generator_hardcoded_music.py
    generator_old_auto.py
    generator_player_prev_next.py
    generator_player_shuffle.py
    generator_thin_client.py
    generator_thin_client_no_audio_client.py
    generator_thin_client_no_plus.py
    generator_user_no_plus.py
    generator_yabro.py
    generator.py
)

SIZE(LARGE)

FORK_SUBTESTS()

SPLIT_FACTOR(2)

DATA(
    arcadia/alice/hollywood/library/scenarios/music/it/data
    arcadia/alice/hollywood/library/scenarios/music/it/data_auto_old
    arcadia/alice/hollywood/library/scenarios/music/it/data_hardcoded_music
    arcadia/alice/hollywood/library/scenarios/music/it/data_navi_unauthorized_user
    arcadia/alice/hollywood/library/scenarios/music/it/data_player_prev_next
    arcadia/alice/hollywood/library/scenarios/music/it/data_player_shuffle
    arcadia/alice/hollywood/library/scenarios/music/it/data_user_no_plus
    arcadia/alice/hollywood/library/scenarios/music/it/data_weekly_promo
    arcadia/alice/hollywood/library/scenarios/music/it/data_yabro
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client_no_audio_client
    arcadia/alice/hollywood/library/scenarios/music/it/thin_client_no_plus
    arcadia/alice/megamind/configs/dev/combinators
)

REQUIREMENTS(ram:32)

END()
