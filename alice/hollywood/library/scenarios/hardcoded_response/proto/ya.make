PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    akhruslan
    g:hollywood
)

PEERDIR(
    alice/library/client/protos
)

SRCS(
    hardcoded_response.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
