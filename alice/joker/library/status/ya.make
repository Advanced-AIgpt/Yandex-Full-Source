LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/library/network
    library/cpp/http/misc
    library/cpp/http/server
)

SRCS(
    status.cpp
)

GENERATE_ENUM_SERIALIZATION(status.h)

END()
