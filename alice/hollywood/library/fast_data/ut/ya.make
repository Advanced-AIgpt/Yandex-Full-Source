UNITTEST_FOR(alice/hollywood/library/fast_data)

OWNER(g:hollywood)

SRCS(
    alice/hollywood/library/fast_data/fast_data_ut.cpp
)

PEERDIR(
    alice/hollywood/library/fast_data/ut/proto
    alice/library/unittest
    library/cpp/protobuf/util
)

END()
