LIBRARY()

OWNER(g:alice)

SRCS(
    data_sync.cpp
)

GENERATE_ENUM_SERIALIZATION(
    data_sync.h
)

PEERDIR(
    alice/bass/libs/fetcher
    alice/bass/util
)

END()
