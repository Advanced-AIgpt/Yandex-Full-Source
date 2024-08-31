PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

PEERDIR(
    alice/megamind/protos/common
    alice/protos/data/scenario/centaur
    alice/protos/data/scenario/centaur/my_screen
    alice/protos/data/scenario/centaur/teasers
    alice/protos/data/scenario/dialogovo
    alice/protos/data/scenario/example
    alice/protos/data/scenario/iot
    alice/protos/data/scenario/music
    alice/protos/data/scenario/news
    alice/protos/data/scenario/order
    alice/protos/data/scenario/objects
    alice/protos/data/scenario/onboarding
    alice/protos/data/scenario/photoframe
    alice/protos/data/scenario/reminders
    alice/protos/data/scenario/route
    alice/protos/data/scenario/search
    alice/protos/data/scenario/weather
    alice/protos/data/scenario/traffic
    alice/protos/data/scenario/video
    alice/protos/data/scenario/video_call
    alice/protos/data/scenario/general_conversation
    alice/protos/data/scenario/afisha
    alice/protos/data/search_result
    mapreduce/yt/interface/protos
)

SRCS(
    data.proto
)

END()
