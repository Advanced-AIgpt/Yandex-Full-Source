LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    enrollment_repository.cpp
    processor.cpp
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging
    alice/megamind/protos/guest
    alice/library/proto

    voicetech/library/messages
    voicetech/library/proto_api

    apphost/api/service/cpp

    voicetech/bio/ondevice/lib/proto_convertor
)

END()

RECURSE_FOR_TESTS(ut)
