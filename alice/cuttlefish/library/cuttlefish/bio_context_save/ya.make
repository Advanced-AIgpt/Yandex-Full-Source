LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.h
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common

    alice/cachalot/api/protos
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging

    alice/megamind/protos/speechkit

    voicetech/library/proto_api

    apphost/api/service/cpp
    apphost/lib/proto_answers
)

END()
