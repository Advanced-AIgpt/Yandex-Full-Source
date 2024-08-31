LIBRARY()

OWNER(
    flimsywhimsy
    lavv17
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/video_musical_clips/nlg
    alice/hollywood/library/scenarios/video_musical_clips/proto
    alice/hollywood/library/scenarios/fast_command
    alice/hollywood/library/scenarios/music
    alice/library/proto
    alice/library/video_common/frontend_vh_helpers
    alice/library/video_common/hollywood_helpers
    alice/megamind/protos/scenarios
)

SRCS(
    musical_clips_prepare_handle.cpp
    musical_clips_render_handle.cpp
    musical_clips_run_handle.cpp
    search_clips_prepare_handle.cpp
    utils.cpp
    vh_prepare_handle.cpp
    GLOBAL video_musical_clips.cpp
)

END()


