UNITTEST_FOR(alice/megamind/library/speechkit)

OWNER(g:megamind)

PEERDIR(
    alice/library/unittest
    alice/megamind/library/testing
)

SRCS(
    request_ut.cpp
)

REQUIREMENTS(ram:9)

END()
