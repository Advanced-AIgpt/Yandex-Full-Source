UNITTEST_FOR(alice/hollywood/shards/common/prod/fast_data/alice_show)

OWNER(
    g:hollywood
    lavv17
)

DEPENDS(
    alice/hollywood/shards/common/prod/fast_data/alice_show
)

SRCS(
    alice/hollywood/shards/common/prod/fast_data/util/alice_show/ut/alice_show_ut.cpp
)

PEERDIR(
    alice/hollywood/library/phrases/testing
    alice/hollywood/library/scenarios/alice_show/proto
    alice/hollywood/library/tags/testing
    alice/library/unittest
    library/cpp/protobuf/util
)

END()
