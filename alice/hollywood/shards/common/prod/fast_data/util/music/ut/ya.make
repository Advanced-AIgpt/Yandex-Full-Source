UNITTEST_FOR(alice/hollywood/shards/common/prod/fast_data/music)

OWNER(
    g:hollywood
    lavv17
)

DEPENDS(
    alice/hollywood/shards/common/prod/fast_data/music
)

SRCS(
    alice/hollywood/shards/common/prod/fast_data/util/music/ut/music_shots_ut.cpp
)

PEERDIR(
    alice/hollywood/library/phrases/testing
    alice/hollywood/library/scenarios/music/proto
    alice/hollywood/library/tags/testing
    alice/library/unittest
    library/cpp/protobuf/util
)

END()
