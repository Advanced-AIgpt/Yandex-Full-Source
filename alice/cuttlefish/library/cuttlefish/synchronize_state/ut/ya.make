UNITTEST()
OWNER(g:voicetech-infra)

SRCS(
    test_basic.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/synchronize_state

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos

    alice/library/json

    apphost/lib/common
    apphost/lib/compression
    apphost/lib/service_testing
)

END()
