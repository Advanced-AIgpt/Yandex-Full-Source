LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/aws
    alice/cuttlefish/library/logging

    alice/cachalot/api/protos
    alice/library/cachalot_cache
    alice/library/json
    alice/library/proto
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/memento/proto
    alice/protos/api/matrix
    alice/protos/api/notificator
    alice/uniproxy/library/protos

    apphost/api/service/cpp
    apphost/lib/proto_answers
)

END()
