OWNER(g:megamind)

UNITTEST_FOR(alice/megamind/library/search)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/experiments
    alice/library/logger
    alice/library/network
    alice/library/unittest
    alice/megamind/library/experiments
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/testing
    alice/megamind/library/util
    library/cpp/geobase
    library/cpp/scheme
    search/begemot/rules/query_factors/proto
)

SRCS(
    request_ut.cpp
)

END()
