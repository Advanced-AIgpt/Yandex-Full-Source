LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/stream_servant_base

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/proto_censor
    alice/cuttlefish/library/protos

    voicetech/library/common
)

END()
