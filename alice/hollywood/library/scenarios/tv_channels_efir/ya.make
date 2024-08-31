LIBRARY()

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/tv_channels_efir/play_tv_channel
    alice/hollywood/library/scenarios/tv_channels_efir/show_tv_channels_gallery
    alice/library/experiments
)

SRCS(
    GLOBAL tv_channels_efir.cpp
    render_handle.cpp
    prepare_handle.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
