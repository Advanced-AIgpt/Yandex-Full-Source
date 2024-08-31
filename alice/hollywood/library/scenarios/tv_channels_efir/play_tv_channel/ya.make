LIBRARY()

OWNER(
    antonfn
    g:vh
)

PEERDIR(
    alice/library/client
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/tv_channels_efir/library
    alice/hollywood/library/scenarios/tv_channels_efir/nlg
    alice/library/network
    alice/library/video_common
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    library/cpp/json/writer
    alice/library/video_common/frontend_vh_helpers
)

SRCS(
    player_setup.cpp
    prepare_handle.cpp
    render_handle.cpp
)

END()
