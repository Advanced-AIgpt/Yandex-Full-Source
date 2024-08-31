UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/session_logs_collector)

OWNER(g:voicetech-infra)

SRCS(
    service_ut.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos
    apphost/lib/service_testing
)

END()
