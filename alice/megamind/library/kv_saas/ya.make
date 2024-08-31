LIBRARY()

OWNER(
    g:megamind
    lavv17
)

PEERDIR(
    alice/library/intent_stats/proto
    alice/library/network
    alice/library/util
    alice/megamind/library/experiments
    alice/megamind/library/request_composite
    alice/megamind/library/request_composite/client
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/quality/user_intents/proto
    search/session/compression
)

SRCS(
    request.cpp
    response.cpp
)

END()
