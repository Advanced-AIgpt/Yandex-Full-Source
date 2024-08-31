PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(danichk)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    generate_uniproxy_url_with_srcrwr.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
)

END()
