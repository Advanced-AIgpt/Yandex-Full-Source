LIBRARY()

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/suggesters/common
    alice/hollywood/library/scenarios/suggesters/movies/proto
    alice/hollywood/library/scenarios/suggesters/nlg
    alice/library/json
    alice/library/proto
    alice/library/restriction_level
    alice/library/util
    alice/library/video_common
    alice/protos/data/video
    library/cpp/resource
)

SRCS(
    GLOBAL handle.cpp
    recommender.cpp
)

END()

RECURSE(
    suggestions_preparer
)

RECURSE_FOR_TESTS(
    ut
)
