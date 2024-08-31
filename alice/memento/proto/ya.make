PROTO_LIBRARY(memento_proto)
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/megamind/protos/proactivity
    alice/protos/data
    alice/protos/data/proactivity
    alice/protos/data/scenario/centaur/my_screen
    alice/protos/data/scenario/centaur/teasers
    alice/protos/data/scenario/music
    alice/protos/data/scenario/order
    alice/protos/data/scenario/reminders
    alice/protos/data/scenario/voiceprint
    mapreduce/yt/interface/protos
)

SRCS(
    api.proto
    device_configs.proto
    user_configs.proto
)

END()

RECURSE_FOR_TESTS(ut)
