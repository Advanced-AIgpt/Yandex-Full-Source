LIBRARY()

OWNER(
    antonfn
    g:vh
)

PEERDIR(
    alice/bass/libs/client
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/mordovia_video_selection
    alice/hollywood/library/scenarios/tv_channels_efir/library
    alice/hollywood/library/scenarios/tv_channels_efir/nlg
    alice/library/experiments
    alice/library/network
    alice/library/video_common
    alice/library/video_common/hollywood_helpers
    alice/megamind/protos/common
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
)

END()
