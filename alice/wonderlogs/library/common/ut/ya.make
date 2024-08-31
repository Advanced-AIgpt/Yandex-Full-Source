UNITTEST()

OWNER(g:wonderlogs)

SRCS(
    invalid_enum_message.proto
    invalid_enum_message3.proto
    utils_ut.cpp
)

PEERDIR(
    alice/library/json
    alice/library/unittest
    alice/wonderlogs/library/common
)

END()
