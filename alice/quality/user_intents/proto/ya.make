PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_quality)

SRCS(
    personal_intents.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
