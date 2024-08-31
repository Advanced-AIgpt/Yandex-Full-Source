LIBRARY()

OWNER(g:alice)

SRCS(
    passport_api.cpp
)

PEERDIR(
    alice/bass/libs/fetcher
    library/cpp/scheme
)

GENERATE_ENUM_SERIALIZATION(
    passport_api.h
)

END()
