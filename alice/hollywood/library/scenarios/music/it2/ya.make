PY3TEST()

OWNER(
    vitvlkv
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
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/scenarios/music/proto
    alice/protos/endpoint/capabilities/bio
    alice/megamind/protos/scenarios
    alice/protos/api/renderer
    contrib/python/PyHamcrest
)

PY_SRCS(thin_client_helpers.py)

TEST_SRCS(
    conftest.py
    tests_ambient_sound.py
    tests_bitrate.py
    tests_change_track_version.py
    tests_div_render.py
    tests_equalizer.py
    tests_extra_promo_available.py
    tests_fairy_tales.py
    tests_fixlist.py
    tests_fm_radio.py
    tests_generative.py
    tests_guest_mode.py
    tests_hardcoded_lite.py
    tests_infinite_feed.py
    tests_like_status.py
    tests_multiroom.py
    tests_multiroom_token.py
    tests_onboarding.py
    tests_player_commands.py
    tests_promo_available.py
    tests_promo_incorrect_billing_response.py
    tests_rup_streams.py
    tests_save_progress.py
    tests_sound_change.py
    tests_tandem.py
    tests_thin_client.py
    # legacy tests not needed :(
    # will be deleted soon
    # tests_thin_multiroom.py
    tests_unauthorized_smart_tv.py
    tests_watch.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/music/it2/billing_data
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_ambient_sound
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_bitrate
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_generative
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_hardcoded_lite
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_onboarding
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_player_commands
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_rup_streams
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_sound_change
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_save_progress
    arcadia/alice/hollywood/library/scenarios/music/it2/tests_thin_client
    arcadia/alice/megamind/protos/common
)

REQUIREMENTS(ram:32)

END()
