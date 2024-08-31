LIBRARY()

OWNER(
    a-square
    akhruslan
    g:hollywood
)

SRCS(
    fast_data.cpp
)

PEERDIR(
    alice/hollywood/library/scenarios/sssss/proto
    alice/library/logger
    contrib/libs/protobuf
    library/cpp/protobuf/util
)

END()

RECURSE_FOR_TESTS(
    ut
)
