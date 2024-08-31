LIBRARY()
OWNER(g:voicetech-infra)

GENERATE_ENUM_SERIALIZATION(base.h)

SRCS(
    base.cpp
)

PEERDIR(
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/protos
)

END()
