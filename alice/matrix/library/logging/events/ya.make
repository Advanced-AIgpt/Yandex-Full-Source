PROTO_LIBRARY()

OWNER(
    g:matrix
)

SRCS(
    events.ev
)

PEERDIR(
    alice/matrix/analytics/protos
    alice/matrix/notificator/library/services/update_connected_clients/protos
    alice/matrix/notificator/library/services/update_device_environment/protos
    alice/matrix/scheduler/library/services/scheduler/protos
    alice/matrix/worker/library/services/worker/protos

    alice/protos/api/matrix
    alice/protos/api/notificator

    alice/megamind/protos/common
    alice/megamind/protos/scenarios

    alice/uniproxy/library/protos

    library/cpp/eventlog/proto
)

EXCLUDE_TAGS(GO_PROTO)
EXCLUDE_TAGS(JAVA_PROTO)

END()
