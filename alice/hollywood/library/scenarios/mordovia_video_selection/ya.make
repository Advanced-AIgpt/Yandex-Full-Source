LIBRARY()

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/environment_state
    alice/hollywood/library/scenarios/mordovia_video_selection/nlg
    alice/library/experiments
    alice/library/network
    alice/library/proto
    alice/library/video_common
    alice/library/video_common/frontend_vh_helpers
    alice/library/video_common/hollywood_helpers
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    library/cpp/json/writer
)

SRCS(
    GLOBAL mordovia.cpp
    prepare_handle.cpp
    render_handle.cpp
    util.cpp
    ott_setup.cpp
)

GENERATE_ENUM_SERIALIZATION(mordovia_tabs.h)

END()

RECURSE_FOR_TESTS(it)
