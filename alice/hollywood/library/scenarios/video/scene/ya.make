LIBRARY()

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/video/nlg
    alice/hollywood/library/scenarios/video/proto
    alice/hollywood/library/registry

    alice/library/analytics/common
    alice/library/proto
    alice/library/video_common
    alice/library/video_common/frontend_vh_helpers
    alice/library/video_common/hollywood_helpers

    alice/nlu/libs/request_normalizer

    alice/protos/api/renderer
    alice/protos/data/video
    alice/protos/data/search_result
    alice/protos/div
    alice/protos/endpoint
    alice/megamind/protos/common

    library/cpp/uri
)

SRCS(
    bass_proxy.cpp
    card_details.cpp
    feature_calculator.cpp
    open_item.cpp
    play_video.cpp
    search.cpp
    voice_buttons.cpp
)

END()
