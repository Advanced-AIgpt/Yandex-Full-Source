PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    g:megamind
    gandarf
    karina-usm
)

PEERDIR(
    alice/megamind/protos/common
    alice/library/censor/protos
    alice/protos/data/proactivity
    dj/lib/proto
    dj/services/alisa_skills/profile/proto/context
    dj/services/alisa_skills/server/proto/data/analytics
    mapreduce/yt/interface/protos
)

SRCS(
    proactivity.proto
)

END()
