LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    asr.cpp
    context.cpp
    megamind.cpp
    session_context.cpp
    tts.cpp
)

PEERDIR(
    alice/cachalot/api/protos
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos
    alice/megamind/api/request
    alice/protos/data
    voicetech/library/proto_api
)

END()

RECURSE_FOR_TESTS(ut)
