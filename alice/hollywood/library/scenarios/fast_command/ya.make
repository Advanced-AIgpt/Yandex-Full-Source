LIBRARY()

OWNER(
    makatunkin
    nkodosov
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/capability_wrapper
    alice/hollywood/library/frame
    alice/hollywood/library/frame_redirect
    alice/hollywood/library/global_context
    alice/hollywood/library/multiroom
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/response
    alice/hollywood/library/s3_animations
    alice/hollywood/library/scenarios/fast_command/nlg
    alice/hollywood/library/sound
    alice/library/analytics/common
    alice/library/experiments
    alice/library/logger
    alice/library/scled_animations
)

SRCS(
    GLOBAL fast_command.cpp
    clock.cpp
    common.cpp
    do_not_disturb.cpp
    fast_command_scenario_run_context.cpp
    frame_redirect.cpp
    media_play.cpp
    media_session.cpp
    multiroom.cpp
    pause.cpp
    sound.cpp
)

END()

RECURSE_FOR_TESTS(
    it
    it2
    nlg/ut
    ut
)
