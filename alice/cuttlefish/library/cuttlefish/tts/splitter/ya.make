LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
    utils.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/stream_servant_base
    alice/cuttlefish/library/cuttlefish/tts/utils

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/aws
    alice/cuttlefish/library/proto_censor
    alice/cuttlefish/library/protos

    library/cpp/digest/md5

    voicetech/library/common
)

END()

RECURSE_FOR_TESTS(ut)
