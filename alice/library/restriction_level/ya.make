LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/restriction_level/protos
)

SRCS(
    restriction_level.cpp
)

GENERATE_ENUM_SERIALIZATION(restriction_level.h)

END()
