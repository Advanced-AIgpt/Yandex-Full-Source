LIBRARY()

OWNER(
    g:smarttv
)

PEERDIR(
    alice/hollywood/library/environment_state
    alice/hollywood/library/framework
    alice/hollywood/library/registry
    alice/hollywood/library/rpc_service
    alice/hollywood/library/scenarios/video/nlg
    alice/hollywood/library/scenarios/video/scene
    alice/hollywood/library/scenarios/video/proto
    alice/hollywood/library/services/response_merger

    alice/library/analytics/common
    alice/library/proto
    alice/library/response_similarity
    alice/library/video_common/hollywood_helpers
    alice/library/video_common/frontend_vh_helpers
    alice/library/search_result_parser
    alice/library/search_result_parser/video

    alice/nlu/libs/request_normalizer

    alice/protos/api/renderer
    alice/protos/data/search_result
    alice/protos/data/video

    util
)

SRCS(
    GLOBAL video.cpp
    GLOBAL video_dispatcher.cpp
    GLOBAL register.cpp
    analytics.cpp
    item_selector.cpp
    video_search.cpp
    video_vh.cpp
    web_search_helpers.cpp
    search_metrics.cpp
)

END()

RECURSE_FOR_TESTS(
    it2
    ut
)

RECURSE(
    scene
)
