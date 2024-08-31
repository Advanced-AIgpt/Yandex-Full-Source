UNITTEST_FOR(alice/hollywood/library/scenarios/suggesters/movies)

OWNER(
    dan-anastasev
    g:megamind
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/resources
    alice/hollywood/library/scenarios/suggesters/common
    alice/hollywood/library/scenarios/suggesters/movies/proto
    alice/hollywood/library/scenarios/suggesters/nlg
    alice/library/json
    alice/library/proto
    alice/library/restriction_level
    alice/library/unittest
    alice/library/util
    alice/library/video_common
    alice/protos/data/video
    apphost/lib/service_testing
    library/cpp/resource
)

DEPENDS(
    alice/hollywood/library/scenarios/suggesters/movies/ut/data
)

SRCS(
    alice/hollywood/library/scenarios/suggesters/movies/handle_ut.cpp
    alice/hollywood/library/scenarios/suggesters/movies/recommender_ut.cpp
)

END()
