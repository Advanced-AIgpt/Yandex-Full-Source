PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:megamind)

INCLUDE_TAGS(GO_PROTO)

PEERDIR(
    alice/library/censor/protos
    alice/megamind/protos/blackbox
    mapreduce/yt/interface/protos
)

SRCS(
    guest_data.proto
    guest_options.proto
    enrollment_headers.proto
)

GENERATE_ENUM_SERIALIZATION(enrollment_headers.pb.h)

END()
