PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice)

SRCS(
    polyglot_translation_request.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
