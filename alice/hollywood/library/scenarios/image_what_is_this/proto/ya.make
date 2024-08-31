PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    polushkin
    g:cv-search
)

SRCS(
    image_what_is_this.proto
    render_request.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
