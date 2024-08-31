LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    base.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging

    apphost/api/service/cpp
)

END()
