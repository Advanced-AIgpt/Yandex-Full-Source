UNITTEST()
OWNER(g:voicetech-infra)

SRCS(
    test_util.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/store_audio
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
)

END()
