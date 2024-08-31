LIBRARY()

OWNER(g:alice)

SRCS(
    enums.cpp
)

PEERDIR(
)

GENERATE_ENUM_SERIALIZATION(
    enums.h
)

END()
