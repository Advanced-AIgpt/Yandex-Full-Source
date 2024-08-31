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

    alice/hollywood/library/scenarios/suggesters/movie_akinator

    alice/hollywood/library/framework
    alice/hollywood/library/gif_card

    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/analytics/scenarios/general_conversation

    alice/library/analytics/scenario
    alice/library/logger
    alice/library/proto
    alice/library/video_common

    library/cpp/iterator
)

SRCS(
    general_conversation_response_builder.cpp
    reply_source_render_strategy.cpp
    reply_sources/aggregated_strategy.cpp
    reply_sources/easter_egg_strategy.cpp
    reply_sources/error_strategy.cpp
    reply_sources/generic_static_strategy.cpp
    reply_sources/generative_tale_strategy.cpp
    reply_sources/generative_toast_strategy.cpp
    reply_sources/movie_akinator_strategy.cpp
    reply_sources/proactivity_strategy.cpp
)

END()
