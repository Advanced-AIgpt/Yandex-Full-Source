GTEST()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/megamind/speaker
    apphost/lib/service_testing
)

SRCS(
    context_ut.cpp
    service_apphosted_ut.cpp
    service_ut.cpp
)

END()
