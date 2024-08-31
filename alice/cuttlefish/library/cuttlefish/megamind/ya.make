LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    megamind.cpp
    service.cpp
    state_machine.cpp
    subrequest.cpp
    worker.cpp
    utils.cpp
)

PEERDIR(
    alice/cuttlefish/library/apphost

    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/config
    alice/cuttlefish/library/cuttlefish/context_save/client
    alice/cuttlefish/library/cuttlefish/megamind/client
    alice/cuttlefish/library/cuttlefish/megamind/mappers
    alice/cuttlefish/library/cuttlefish/megamind/request
    alice/cuttlefish/library/cuttlefish/megamind/speaker
    alice/cuttlefish/library/cuttlefish/stream_converter

    alice/cachalot/api/protos
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/proto_censor

    alice/library/typed_frame

    voicetech/asr/engine/proto_api
    voicetech/library/itags
    voicetech/library/messages

    apphost/api/service/cpp
    apphost/lib/proto_answers
)

END()

RECURSE(
    client
    request
)

RECURSE_FOR_TESTS(ut)
