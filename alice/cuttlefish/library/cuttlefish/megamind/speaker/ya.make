LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    context.cpp
    service_apphosted.cpp
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/protos
    alice/library/blackbox
    alice/library/json
    alice/megamind/protos/blackbox
    alice/megamind/protos/guest
    apphost/lib/proto_answers
)

END()

RECURSE_FOR_TESTS(ut)