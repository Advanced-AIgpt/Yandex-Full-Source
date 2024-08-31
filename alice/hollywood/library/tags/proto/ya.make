PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    lavv17
    g:alice
)

PEERDIR(
    alice/library/proto_eval/proto
)

SRCS(
    tags.proto
)

END()
