PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    gusev-p
    g:kwyt
)

SRCS(
    megamind_log.proto
    rtlog.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
    robot/rthub/yql/generic_protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
