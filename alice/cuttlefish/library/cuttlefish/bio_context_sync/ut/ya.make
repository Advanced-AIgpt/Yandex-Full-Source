GTEST()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/bio_context_sync
    apphost/lib/service_testing
)

SRCS(
    processor_ut.cpp
    service_ut.cpp
)

END()
