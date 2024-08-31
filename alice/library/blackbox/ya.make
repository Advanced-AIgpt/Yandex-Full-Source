LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/library/blackbox/proto
    alice/library/network
    alice/library/util
)

SRCS(
    blackbox.cpp
    blackbox_http.cpp
    status.cpp
)

GENERATE_ENUM_SERIALIZATION(blackbox.h)
GENERATE_ENUM_SERIALIZATION(status.h)

END()

RECURSE_FOR_TESTS(ut)
