LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    converters.cpp
    converters.h
    service.cpp
    service.h
    utils.h
)

PEERDIR(
    alice/cuttlefish/library/convert
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_converters
    alice/cuttlefish/library/protos

    voicetech/library/messages

    apphost/api/service/cpp

    library/cpp/json
)

END()

RECURSE_FOR_TESTS(ut)
