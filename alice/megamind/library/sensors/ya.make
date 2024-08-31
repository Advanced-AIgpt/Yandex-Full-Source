LIBRARY()

OWNER(
    g:megamind
)

PEERDIR(
    alice/megamind/library/context
    alice/megamind/library/requestctx
)

SRCS(
    utils.cpp
)

GENERATE_ENUM_SERIALIZATION(utils.h)

END()
