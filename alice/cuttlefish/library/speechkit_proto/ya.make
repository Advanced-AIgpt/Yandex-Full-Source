PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:voicetech-infra)

SRCS(
    client_ws.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
