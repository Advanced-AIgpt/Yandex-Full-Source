PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(danichk)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    build_marker_tests_config.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
)

END()
