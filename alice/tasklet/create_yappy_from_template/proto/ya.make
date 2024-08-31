PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(danichk)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    create_yappy_from_template.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
    search/priemka/yappy/proto
)

END()
