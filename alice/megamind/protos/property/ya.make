PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/memento/proto

    mapreduce/yt/interface/protos
)

SRCS(
    alice_show_profile.proto
    iot_profile.proto
    morning_show_profile.proto
    property.proto
)

END()
