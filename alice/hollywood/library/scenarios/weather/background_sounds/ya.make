LIBRARY()

OWNER(
    sparkle
    g:alice
)

PEERDIR(
    alice/library/util
)

SRCS(
    background_sounds.cpp
)

GENERATE_ENUM_SERIALIZATION(background_sounds.h)

END()

RECURSE_FOR_TESTS(
    ut
)
