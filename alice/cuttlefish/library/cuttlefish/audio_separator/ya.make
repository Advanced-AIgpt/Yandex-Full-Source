LIBRARY()
OWNER(g:voicetech-infra)

GENERATE_ENUM_SERIALIZATION(servant.h)

SRCS(
    servant.cpp
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/stream_servant_base

    alice/cuttlefish/library/protos
)

END()
