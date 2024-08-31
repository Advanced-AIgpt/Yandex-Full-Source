PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_fun)

SRCS(
    differ_report.proto
    extension.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

END()
