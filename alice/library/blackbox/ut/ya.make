UNITTEST_FOR(alice/library/blackbox)

OWNER(g:alice)

PEERDIR(
    alice/library/network
    alice/library/unittest
    library/cpp/testing/gmock_in_unittest
)

SRCS(
    blackbox_ut.cpp
)

END()
