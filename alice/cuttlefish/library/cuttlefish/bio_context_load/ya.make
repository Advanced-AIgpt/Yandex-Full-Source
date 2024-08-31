LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging

    alice/library/proto

    voicetech/library/messages
    voicetech/library/proto_api

    apphost/api/service/cpp
)

END()
