UNITTEST_FOR(alice/megamind/api/request)

OWNER(
    alkapov
    g:megamind
)

SRCS(
    constructor_ut.cpp
)

PEERDIR(
    alice/library/json
    alice/library/unittest
)

REQUIREMENTS(ram:10)

END()
