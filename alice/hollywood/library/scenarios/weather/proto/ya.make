PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    sparkle
    g:alice
)

PEERDIR(
    alice/megamind/protos/common
)

SRCS(
    weather.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
