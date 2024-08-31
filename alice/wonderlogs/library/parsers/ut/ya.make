UNITTEST()

OWNER(g:wonderlogs)

SRCS(
    megamind_ut.cpp
    uniproxy_ut.cpp
    wonderlogs_ut.cpp
)

PEERDIR(
    alice/library/unittest

    alice/wonderlogs/library/parsers
)

END()
