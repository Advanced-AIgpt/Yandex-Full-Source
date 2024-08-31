UNION()

OWNER(
    tolyandex
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TNotificationsFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/notifications/notifications.pb.txt
    notifications.pb
    IN alice/hollywood/shards/common/prod/fast_data/notifications/notifications.pb.txt
    OUT notifications.pb
)

END()
