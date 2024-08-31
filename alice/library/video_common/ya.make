OWNER(
    g:alice
    g:smarttv
)

LIBRARY()

SRCS(
    age_restriction.cpp
    defs.cpp
    device_helpers.cpp
    mordovia_webview_helpers.cpp
    mordovia_webview_defs.cpp
    vh_player.cpp
    video_helper.cpp
    video_provider.cpp
    audio_and_subtitle_helper.cpp
)

PEERDIR(
    alice/library/json
    alice/library/proto
    alice/library/restriction_level
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data/search_result
    alice/protos/data/video
    contrib/libs/protobuf
    library/cpp/json/writer
    library/cpp/scheme
)

GENERATE_ENUM_SERIALIZATION(defs.h)

END()

RECURSE_FOR_TESTS(
    ut
)
