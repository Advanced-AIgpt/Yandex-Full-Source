UNITTEST_FOR(alice/megamind/library/handlers/apphost_utility)

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/logger
    alice/library/unittest

    alice/megamind/library/testing

    library/cpp/logger
    library/cpp/neh
    library/cpp/testing/unittest
)


SRCS(
    handler_ut.cpp
)

END()
