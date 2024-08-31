UNITTEST_FOR(alice/nlu/libs/type_parser)

OWNER(
    g:alice_quality
    smirnovpavel
)

FROM_SANDBOX(
    1135121233 OUT type_parser_time_rus.dict
)

RESOURCE(
    type_parser_time_rus.dict type_parser_time_rus.dict
)

PEERDIR(
    library/cpp/testing/unittest
)

SRCS(
    type_parser_ut.cpp
)

END()
