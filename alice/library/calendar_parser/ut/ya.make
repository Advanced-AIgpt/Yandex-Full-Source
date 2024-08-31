UNITTEST_FOR(alice/library/calendar_parser)

OWNER(
    g:alice
    g:alice-alarm-scenario
)

PEERDIR(
    alice/library/unittest
)

SRCS(
    iso8601_ut.cpp
    parser_ut.cpp
    reader_ut.cpp
    types_ut.cpp
)

END()
