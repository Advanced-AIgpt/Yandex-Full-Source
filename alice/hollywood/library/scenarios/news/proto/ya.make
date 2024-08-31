PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:hollywood
    khr2
)

SRCS(
    apply_arguments.proto
    news_fast_data.proto
    news.proto
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/memento/proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
