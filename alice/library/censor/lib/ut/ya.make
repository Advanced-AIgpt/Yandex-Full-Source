UNITTEST_FOR(alice/library/censor/lib)

OWNER(g:alice_fun)

SRCS(
    censor_ut.cpp
    private_message.proto
)

PEERDIR(
    alice/library/unittest
)

END()
