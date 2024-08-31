PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

SRCS(
    invalid_request.proto
)


EXCLUDE_TAGS(GO_PROTO)

END()

