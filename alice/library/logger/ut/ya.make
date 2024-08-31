UNITTEST_FOR(alice/library/logger)

OWNER(g:alice)

PEERDIR(
    alice/library/unittest
    library/cpp/logger
)

SRCS(
    logger_ut.cpp
)

END()
