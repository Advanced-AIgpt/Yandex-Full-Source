UNITTEST_FOR(alice/cuttlefish/library/cuttlefish/context_save/client)

OWNER(g:voicetech-infra)

SRCS(
    common.cpp
    starter_ut.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    apphost/lib/service_testing
)

END()
