LIBRARY()

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/candidates
    alice/hollywood/library/scenarios/general_conversation/common
    alice/hollywood/library/scenarios/general_conversation/containers
    alice/hollywood/library/scenarios/general_conversation/classification
    alice/hollywood/library/scenarios/general_conversation/long_session
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/hollywood/library/scenarios/general_conversation/response_builders

    alice/hollywood/library/scenarios/suggesters/movie_akinator

    alice/hollywood/library/framework
    alice/hollywood/library/gif_card

    alice/protos/data/language
    alice/megamind/protos/scenarios
    alice/megamind/protos/analytics/scenarios/general_conversation

    alice/library/analytics/scenario
    alice/library/logger
    alice/library/proto
    alice/library/video_common

    alice/paskills/social_sharing/proto/api

    library/cpp/iterator
    library/cpp/string_utils/base64
)

SRCS(
    continue_init_handle.cpp
    continue_candidates_aggregator_handle.cpp
    continue_candidates_handle.cpp
    continue_render_handle.cpp
    prepare_commit_social_sharing_link.cpp
    run_init_handle.cpp
    run_candidates_aggregator_handle.cpp
    run_candidates_handle.cpp
    run_render_handle.cpp
)

END()
