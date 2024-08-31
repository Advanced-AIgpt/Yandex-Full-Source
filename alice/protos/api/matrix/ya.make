PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:matrix
)

SRCS(
    action.proto
    client_connections.proto
    delivery.proto
    device_environment.proto
    schedule_action.proto
    scheduled_action.proto
    scheduler_api.proto
    technical_push.proto
    user_device.proto
)

PEERDIR(
    alice/megamind/protos/common

    mapreduce/yt/interface/protos
)

END()
