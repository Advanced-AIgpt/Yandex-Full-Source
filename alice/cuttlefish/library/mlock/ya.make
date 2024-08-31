LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    mlock.cpp
)

PEERDIR(
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/protos
)

END()
