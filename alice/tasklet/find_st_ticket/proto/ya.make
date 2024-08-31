PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(danichk)

TASKLET()

EXCLUDE_TAGS(GO_PROTO)

SRCS(
    find_st_ticket.proto
)

PEERDIR(
    ci/tasklet/common/proto
    tasklet/api
    tasklet/services/ci/proto
    tasklet/services/yav/proto
)

END()
