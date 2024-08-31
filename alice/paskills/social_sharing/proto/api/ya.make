OWNER(g:paskills)

PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

EXCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    apphost/proto/extensions
)

SRCS(
    api.proto
    directives.proto
    web_api.proto
    web_page.proto
)

END()
