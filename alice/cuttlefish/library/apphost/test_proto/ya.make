PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:voicetech-infra)

SRCS(
    test.proto
)

CPP_PROTO_PLUGIN(
    prototraits alice/cuttlefish/tools/prototraits .traits.pb.h
)

EXCLUDE_TAGS(GO_PROTO)

END()
