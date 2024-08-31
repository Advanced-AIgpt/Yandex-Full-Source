UNITTEST_FOR(alice/library/search_result_parser/video)

OWNER(g:smarttv)

PEERDIR(
    alice/library/json
    alice/library/proto
    alice/library/proto/ut/protos
    library/cpp/testing/gmock_in_unittest
)

SRCS(parser_util_ut.cpp)

DATA(
    sbr://3229132062=fixtures/ # entity_search snippet from Web DS by request "Смычок фильм" dumped in json :  Smychok.json
)

END()
