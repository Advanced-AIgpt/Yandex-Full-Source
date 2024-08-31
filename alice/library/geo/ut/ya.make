UNITTEST_FOR(alice/library/geo)

OWNER(
    sparkle
    g:megamind
)

PEERDIR(
    alice/library/json
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    alice/library/geo/geodb_ut.cpp
)

END()
