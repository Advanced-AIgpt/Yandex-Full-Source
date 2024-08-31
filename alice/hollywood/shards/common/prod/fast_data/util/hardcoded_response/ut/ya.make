UNITTEST_FOR(alice/hollywood/shards/common/prod/fast_data/hardcoded_response)

OWNER(g:hollywood)

DATA(
    arcadia/alice/hollywood/shards/common/prod/fast_data/hardcoded_response
)

SRCS(
    alice/hollywood/shards/common/prod/fast_data/util/hardcoded_response/ut/hardcoded_response_ut.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/hardcoded_response/proto
    alice/library/unittest
    library/cpp/protobuf/util
)

END()
