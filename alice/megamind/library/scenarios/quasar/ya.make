LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/video_common
    alice/megamind/library/context
)

SRCS(
    common.cpp
)

GENERATE_ENUM_SERIALIZATION(common.h)

END()
