PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(danichk)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    remove_old_beta.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
    search/priemka/yappy/proto
)

END()
