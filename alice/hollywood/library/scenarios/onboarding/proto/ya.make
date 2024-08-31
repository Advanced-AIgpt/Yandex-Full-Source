PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(
    vitamin-ca
    g:alice
)

PEERDIR(
    dj/services/alisa_skills/server/proto/data
)

SRCS(
    onboarding.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
