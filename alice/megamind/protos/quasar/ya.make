OWNER(g:quasar_infra g:megamind)

PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    mapreduce/yt/interface/protos
)

SRCS(
    auxiliary_config.proto
)

END()
