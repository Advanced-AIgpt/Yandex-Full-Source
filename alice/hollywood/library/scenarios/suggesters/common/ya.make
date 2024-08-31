LIBRARY()

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/suggesters/nlg
    alice/library/util
    alice/library/video_common
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    library/cpp/iterator
)

SRCS(
    base_suggest_handle.cpp
    recommender_utils.cpp
    state_updater.cpp
    suggest_response_builder.cpp
    utils.cpp
)

END()
