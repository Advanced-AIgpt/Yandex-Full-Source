LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.h
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging

    apphost/api/service/cpp
    apphost/lib/proto_answers
)

END()
