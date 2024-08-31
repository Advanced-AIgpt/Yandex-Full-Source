UNITTEST_FOR(alice/library/search_result_parser)

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/library/json
    alice/library/proto_eval/proto
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    search_result_parser_ut.cpp
)

END()
