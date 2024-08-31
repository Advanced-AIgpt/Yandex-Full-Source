UNITTEST_FOR(alice/hollywood/library/scenarios/suggesters/games)

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/resources
    alice/hollywood/library/scenarios/suggesters/common
    alice/hollywood/library/scenarios/suggesters/games/proto
    alice/hollywood/library/scenarios/suggesters/nlg
    alice/library/frame
    alice/library/json
    alice/library/unittest
    alice/library/util
    apphost/lib/service_testing
    library/cpp/resource
)

DEPENDS(
    alice/hollywood/library/scenarios/suggesters/games/ut/data
)

SRCS(
    alice/hollywood/library/scenarios/suggesters/games/handle_ut.cpp
)

END()
