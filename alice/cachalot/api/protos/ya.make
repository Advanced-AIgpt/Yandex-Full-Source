PROTO_LIBRARY()
SET(BUILD_PROTO_AS_EVLOG "yes")
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:voicetech-infra
)

SRCS(
    cachalot.proto
)

PEERDIR(
    library/cpp/eventlog/proto
)

EXCLUDE_TAGS(GO_PROTO)
GENERATE_ENUM_SERIALIZATION(cachalot.pb.h)

END()
