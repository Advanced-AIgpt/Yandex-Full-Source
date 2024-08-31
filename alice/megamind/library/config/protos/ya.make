PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    makatunkin
    g:megamind
)

PEERDIR(
    alice/library/logger/proto
    alice/megamind/library/classifiers/formulas/protos
    alice/megamind/protos/common
    infra/libs/logger/protos
    infra/libs/udp_metrics/client/config
    library/cpp/getoptpb/proto
)

SRCS(
    classification_config.proto
    config.proto
    extensions.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
